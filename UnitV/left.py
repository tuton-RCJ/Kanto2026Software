import sensor, image, time
import KPU as kpu
import gc, sys
from machine import UART
import ustruct
from fpioa_manager import fm

USE_DOUBLE_BUF = True

DEBUG_EVERY = 1
GC_EVERY = 30
COLOR_EVERY = 1

MIN_CONF = 0.80
CHAR_CONSISTENT = 2
CHAR_BINARY_THRESHOLD = 50

COLOR_PIXELS_THR = 120
COLOR_AREA_THR = 120
COLOR_HISTORY_N = 2

CENTER_ZOOM_SCALE = 1.2

last_character_label = None
character_count = 0

COLOR_NAMES = ("Black", "Red", "Yellow", "Green", "Blue")
COLOR_TARGETS = (
    (20, 20, 20),
    (240, 50, 30),
    (220, 180, 40),
    (40, 120, 20),
    (50, 130, 205),
)

DETECT_LAB_THRESHOLDS = [
(0, 30, -15, 12, -40, 15), # Black lab_mean=(4,-2,3) lab_sigma=(3,4,4)
(30, 75, 30, 90, 40, 90), # Red lab_mean=(48,78,61) lab_sigma=(5,9,6)
(30, 85, -20, 50, 55, 100), # Yellow
(30, 75, -60, -20, 10, 70), # Green lab_mean=(56,-44,13) lab_sigma=(5,14,17)
(35, 75, -40, 40, -65, -30), # Blue lab_mean=(61,7,-43) lab_sigma=(5,15,20)
]

DETECT_YELLOW_INDEX = 2
DETECT_SEED_CODE_MASK = 0xFFFF ^ (1 << DETECT_YELLOW_INDEX)

labels = ("Omega", "Psi", "Phi", "Other")
CHAR_CODES = {"Omega": 1, "Psi": 2, "Phi": 3}

color_history = []
last_color_debug = "No color"
center_zoom_enabled = False

fm.register(35, fm.fpioa.UART1_TX, force=True)
fm.register(34, fm.fpioa.UART1_RX, force=True)
uart = UART(UART.UART1, 115200, 8, None, 1, timeout=1000, read_buf_len=4096)

gc.enable()
gc.threshold(gc.mem_alloc() + gc.mem_free())


def send(data):
    uart.write(ustruct.pack("B", data & 0xFF))


def update_center_zoom_state():
    global center_zoom_enabled

    while uart.any():
        cmd = uart.readchar()
        if cmd == 1:
            center_zoom_enabled = True
        elif cmd == 0:
            center_zoom_enabled = False

    return center_zoom_enabled


def center_zoom_image(img):
    w = img.width()
    h = img.height()

    crop_h = int((h / CENTER_ZOOM_SCALE) + 0.5)

    if crop_h < 1:
        crop_h = 1

    y = (h - crop_h) // 2

    croped = img.crop(
        roi=(0, y, w, crop_h),
        x_scale=1,
        y_scale=(float(h) + 0.01) / crop_h,
    )
    return croped


def snap():
    try:
        return sensor.snapshot()
    except RuntimeError:
        return None


def preprocess_character_image(img):
    img.to_grayscale()
    img.binary([(CHAR_BINARY_THRESHOLD, 255)])
    #img.remove_edge_connected_black()
    return img

def detect_character_victim_from_fmap(fmap):
    global last_character_label, character_count

    v0 = fmap[0]
    v1 = fmap[1]
    v2 = fmap[2]
    v3 = fmap[3]

    pmax = v0
    max_index = 0
    if v1 > pmax:
        pmax = v1; max_index = 1
    if v2 > pmax:
        pmax = v2; max_index = 2
    if v3 > pmax:
        pmax = v3; max_index = 3

    label = labels[max_index].strip()
    if pmax <= MIN_CONF or label == "Other":
        last_character_label = None
        character_count = 0
        return 0

    if last_character_label == label:
        character_count += 1
    else:
        last_character_label = label
        character_count = 1

    if character_count >= CHAR_CONSISTENT:
        character_count = CHAR_CONSISTENT - 1 if CHAR_CONSISTENT > 1 else 0
        return CHAR_CODES[label]
    return 0


def push_color_history(layers, conf):
    color_history.append((tuple(layers), conf))
    if len(color_history) > COLOR_HISTORY_N:
        color_history.pop(0)


def clear_color_history():
    while color_history:
        color_history.pop()


def get_stable_color_layers():
    if len(color_history) <= 0:
        return None, 0.0

    stable = []
    layer_confs = []

    for layer_i in range(5):
        votes = [0.0, 0.0, 0.0, 0.0, 0.0]
        total = 0.0

        for layers, conf in color_history:
            votes[layers[layer_i]] += conf
            total += conf

        if total <= 0.0:
            return None, 0.0

        best = 0
        for i in range(1, 5):
            if votes[i] > votes[best]:
                best = i

        stable.append(best)
        layer_confs.append(votes[best] / total)

    min_conf = layer_confs[0]
    for c in layer_confs:
        if c < min_conf:
            min_conf = c

    return stable, min_conf


def detect_colored_victim(img):
    global last_color_debug

    blobs = img.find_blobs_any(
        DETECT_LAB_THRESHOLDS,
        pixels_threshold=COLOR_PIXELS_THR,
        area_threshold=COLOR_AREA_THR,
        merge=True,
        margin=5,
        seed_code_mask=DETECT_SEED_CODE_MASK,
    )

    if not blobs:
        clear_color_history()
        last_color_debug = "No blob"
        return
    blobs_res = []
    for b in blobs:
        x, y, w, h = b.rect()
        if 0.9 > b.pixels()/(w * h) > 0.45 and w <= 70 and h <= 70 and min(h/w, w/h) > 0.65 and y > 5 and y < 107:
            blobs_res.append(b)

    if not blobs_res:
        clear_color_history()
        last_color_debug = "No target-shaped blob"
        return
    target = max(blobs_res, key=lambda b: b.pixels())
    x, y, w, h = target.rect()
    if w < 20 or h < 20:
        clear_color_history()
        last_color_debug = "Small blob:%s" % (target.rect(),)
        return

    cx = target.cx()
    cy = target.cy()
    rx = float(w) * 0.5
    ry = float(h) * 0.5
    left_margin = cx - rx
    right_margin = img.width() - 1 - (cx + rx)
    top_margin = cy - ry
    bottom_margin = img.height() - 1 - (cy + ry)

    result = img.estimate_concentric_score(
        cx,
        cy,
        rx,
        ry,
        targets=COLOR_TARGETS,
    )
    if not result:
        clear_color_history()
        last_color_debug = "No score rect:%s margin:%d,%d,%d,%d" % (
            target.rect(),
            int(left_margin),
            int(right_margin),
            int(top_margin),
            int(bottom_margin),
        )
        return

    score, layers, final_conf, center_layers, center_conf, width_layers, width_conf = result
    push_color_history(layers, final_conf)

    stable_layers, stable_conf = get_stable_color_layers()
    if (not stable_layers) or all([i == 2 for i in stable_layers]) :
        clear_color_history()
        last_color_debug = "No stable color"
        return

    stable_score = 0
    for c in stable_layers:
        stable_score += (-2, -1, 0, 1, 2)[c]

    last_color_debug = "raw:%d layers:%s conf:%d rect:%s center:%d,%d" % (
        stable_score,
        layers,
        int(stable_conf * 100),
        target.rect(),
        cx,
        cy,
    )
    if stable_score < 0 or stable_score >= 3:
        clear_color_history()
        return
    return (stable_score + 4)


def snap_for_character():
    img = snap()
    if img is None:
        return None
    return preprocess_character_image(img)


def setup_sensor():
    sensor.reset(dual_buff=USE_DOUBLE_BUF)
    sensor.set_jb_quality(1)
    sensor.set_pixformat(sensor.RGB565)
    sensor.set_framesize(sensor.QQVGA)
    sensor.set_auto_gain(False, 50)
    sensor.set_windowing((20,0,112,112))
    sensor.set_auto_exposure(False, 3000)
    sensor.set_auto_whitebal(False,(74, 64, 89))
    sensor.run(1)
    sensor.skip_frames(time=1500)


def main(model_addr=0x300000):
    setup_sensor()

    task = kpu.load(model_addr)
    clock = time.clock()

    frame_i = 0
    try:
        while True:
            clock.tick()
            frame_i += 1

            zoom_active = update_center_zoom_state()
            img = snap()
            if img is None:
                send(0)
                continue

            if zoom_active:
                img = center_zoom_image(img)

            col = detect_colored_victim(img)
            print(col)
            preprocess_character_image(img)
            fmap = kpu.forward(task, img)
            ch = detect_character_victim_from_fmap(fmap)

            if ch:
                send(ch)
            elif col:
                send(col)
            else:
                send(0)

            if DEBUG_EVERY > 0 and (frame_i % DEBUG_EVERY) == 0:
                print(clock.fps(), ch, col, last_color_debug)

            if GC_EVERY > 0 and (frame_i % GC_EVERY) == 0:
                gc.collect()

    finally:
        kpu.deinit(task)


if __name__ == "__main__":
    try:
        main()
    except Exception as e:
        sys.print_exception(e)
    finally:
        gc.collect()
