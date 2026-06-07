import sensor, image, time
import KPU as kpu
import gc, sys
from machine import UART
import ustruct
from fpioa_manager import fm

USE_DOUBLE_BUF = False

DEBUG_EVERY = 1      
GC_EVERY = 30       
COLOR_EVERY = 1   

MIN_CONF = 0.7
CHAR_CONSISTENT = 3  
CHAR_BINARY_THRESHOLD = 60

COLOR_PIXELS_THR = 120
COLOR_AREA_THR = 120
COLOR_HISTORY_N = 2


last_character_label = None
character_count = 0

COLOR_NAMES = ("Black", "Red", "Yellow", "Green", "Blue")
COLOR_TARGETS = (
    (40, 40, 40),
    (200, 50, 50),
    (200, 200, 50),
    (10, 90, 60),
    (50, 80, 200),
)

DETECT_LAB_THRESHOLDS = [
    (0, 20, -30, 30, -50, 50),      # Black
    (20, 80, 30, 80, 30, 70),       # Red
    (30, 100, -40, 40, 40, 90),     # Yellow
    (10, 60, -60, -20, 0, 50),      # Green
    (30, 70, 10, 40, -100, -50),    # Blue
]

labels = ("Omega", "Psi", "Phi", "Other")
CHAR_CODES = {"Omega": 1, "Psi": 2, "Phi": 3}

color_history = []

fm.register(35, fm.fpioa.UART1_TX, force=True)
fm.register(34, fm.fpioa.UART1_RX, force=True)
uart = UART(UART.UART1, 115200, 8, None, 1, timeout=1000, read_buf_len=4096)

gc.enable()
gc.threshold(gc.mem_alloc() + gc.mem_free())


def send(data):
    uart.write(ustruct.pack("B", data & 0xFF))


def snap():
    try:
        return sensor.snapshot()
    except RuntimeError:
        return None


def preprocess_character_image(img):
    img.to_grayscale()
    img.binary([(CHAR_BINARY_THRESHOLD, 255)])
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
    blobs = img.find_blobs(
        DETECT_LAB_THRESHOLDS,
        pixels_threshold=COLOR_PIXELS_THR,
        area_threshold=COLOR_AREA_THR,
        merge=True,
        margin=5,
    )

    if not blobs:
        return 0

    target = max(blobs, key=lambda b: b.pixels())
    x, y, w, h = target.rect()
    if w < 20 or h < 20:
        return 0

    result = img.estimate_concentric_score(
        target.cx(),
        target.cy(),
        float(w) * 0.5,
        float(h) * 0.5,
        targets=COLOR_TARGETS,
    )
    if not result:
        return 0

    score, layers, final_conf, center_layers, center_conf, width_layers, width_conf = result
    push_color_history(layers, final_conf)

    stable_layers, stable_conf = get_stable_color_layers()
    if not stable_layers:
        return 0

    stable_score = 0
    for c in stable_layers:
        stable_score += (-2, -1, 0, 1, 2)[c]

    if stable_score < 0 or stable_score >= 3:
        return 0
    return stable_score + 1


def snap_for_character():
    img = snap()
    if img is None:
        return None
    return preprocess_character_image(img)


def setup_sensor():
    sensor.reset(dual_buff=USE_DOUBLE_BUF)
    sensor.set_framesize(sensor.QQVGA)
    sensor.set_pixformat(sensor.RGB565)
    sensor.set_auto_gain(False, 50)
    sensor.set_windowing((46,0,112,112))
    sensor.set_auto_exposure(False, 8000)
    sensor.set_auto_whitebal(False,(82,64,110))
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

            img = snap()
            if img is None:
                send(0)
                continue

            col = 0
            if COLOR_EVERY > 0 and (frame_i % COLOR_EVERY) == 0:
                col = detect_colored_victim(img)

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
                print(clock.fps(), ch, col)

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
