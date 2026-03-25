import sensor, image, time
import KPU as kpu
import gc, sys
from machine import UART
import ustruct
from fpioa_manager import fm

# ====== Tunables for max FPS ======
USE_DOUBLE_BUF = True

DEBUG_EVERY = 1      # 0 = no prints
GC_EVERY = 30        # collect every N frames
COLOR_EVERY = 1      # run color detect every N frames (1 = every frame)

MIN_CONF = 0.9
CHAR_CONSISTENT = 3  # 1 = no debounce (fastest)

COLOR_STRIDE = 8
COLOR_PIXELS_THR = 200

# ================================

last_character_label = None
last_color_label = None
character_count = 0
color_count = 0

thre_green = [(20,70, -40, -20, 5, 20)]
thre_red = [(30, 60, 55, 80, 40, 70)]
thre_yellow = [(35, 100, 0, 30, 50, 75)]

labels = ("H", "None", "S", "U")
CHAR_CODES = {"U": 1, "S": 2, "H": 3}
COLOR_CODES = {"G": 4, "Y": 5, "R": 6}

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
    if pmax <= MIN_CONF or label == "None":
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


def detect_colored_victim(img):
    global last_color_label, color_count

    blobs = img.find_blobs(
        thre_red + thre_green + thre_yellow,
        pixels_threshold=COLOR_PIXELS_THR,
        merge=True,
        x_stride=COLOR_STRIDE,
        y_stride=COLOR_STRIDE
    )
    if blobs:
        found_code = 0
        for b in blobs:
            found_code |= b.code()

        label = None
        if found_code & 0x01:
            label = "R"
        elif found_code & 0x02:
            label = "G"
        elif found_code & 0x04:
            label = "Y"

        if label is not None:
            if last_color_label == label:
                color_count += 1
            else:
                last_color_label = label
                color_count = 1

            if color_count >= 3:
                color_count = 0
                return COLOR_CODES[label]
            return 0

    last_color_label = None
    color_count = 0
    return 0


def main(model_addr=0x300000):
    sensor.reset(dual_buff=False)
    sensor.set_pixformat(sensor.RGB565)
    sensor.set_framesize(sensor.QQVGA)
    sensor.set_auto_whitebal(False, (76.0, 64.0, 101.0))
    sensor.set_auto_gain(False, gain_db=20)
    sensor.set_auto_exposure(False, exposure_us=8000)
    sensor.set_windowing((0, 0, 112, 112))
    sensor.skip_frames(time=2000)

    task = kpu.load(model_addr)
    clock = time.clock()

    img = snap()
    if img is None:
        return

    kpu.forward_async(task, img)

    frame_i = 0
    try:
        while True:
            clock.tick()
            frame_i += 1

            col = 0
            if COLOR_EVERY > 0 and (frame_i % COLOR_EVERY) == 0:
                col = detect_colored_victim(img)

            kpu.wait_done()
            next_img = snap()


            fmap = kpu.get_output(task, 0)
            ch = detect_character_victim_from_fmap(fmap)

            if ch:
                send(ch)
            elif col:
                send(col)
            else:
                send(0)

            if next_img is not None:
                img = next_img

            kpu.forward_async(task, img)

            if DEBUG_EVERY > 0 and (frame_i % DEBUG_EVERY) == 0:
                print(clock.fps(),ch,col)

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
