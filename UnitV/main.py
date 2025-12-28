import sensor, image, time
import KPU as kpu
import gc, sys
from machine import UART
import ustruct
from fpioa_manager import fm

last_character_label = None
last_color_label = None
character_count = 0
color_count = 0
min_confidence = 0.9
thre_green = [(30,100, -60, -25, -5, 20)]
thre_red = [(20, 100
, 40, 70, 30, 70)]
thre_yellow = [(50, 100, -30, -10, 30, 70)]
task = None

fm.register(35, fm.fpioa.UART1_TX, force=True)
fm.register(34, fm.fpioa.UART1_RX, force=True)

uart = UART(UART.UART1, 115200,8, None, 1, timeout=1000, read_buf_len=4096)

labels = ("H", "None", "S", "U")
sensor_window=(224, 224)

# UART codes (1 byte)
CHAR_CODES = {"U": 1, "S": 2, "H": 3}
COLOR_CODES = {"G": 4, "Y": 5, "R": 6}



gc.enable()
gc.threshold(gc.mem_alloc() + gc.mem_free())

def send(data):
    uart.write(ustruct.pack("B", data))

def detect_character_victim(img):
    global last_character_label
    global character_count
    global task

    fmap = kpu.forward(task, img)
    plist=fmap[:]
    pmax=max(plist)
    max_index=plist.index(pmax)
    label = labels[max_index].strip()
    img.draw_string(0,0, "%.2f : %s" %(pmax, label), scale=2, color=(0, 0, 0))
    #print(label)
    if pmax <= min_confidence:
        last_character_label = None
        character_count = 0
        return None

    if label == "None":
        last_character_label = None
        character_count = 0
        return 0

    if last_character_label == label:
        character_count += 1
    else:
        last_character_label = label
        character_count = 1

    # Debounce: require 3 consecutive frames
    if character_count == 1:
        character_count = 0
        return CHAR_CODES[label]
    return 0

def detect_colored_victim(img):
    global last_color_label
    global color_count
    if img.find_blobs(thre_red, pixels_threshold=200, merge=True):
        if last_color_label == "R":
            color_count += 1
        else:
            last_color_label = "R"
            color_count = 1
        if color_count == 3:
            color_count = 2
            return COLOR_CODES["R"]
        return 0
    elif img.find_blobs(thre_green, pixels_threshold=200 , merge=True):
        if last_color_label == "G":
            color_count += 1
        else:
            last_color_label = "G"
            color_count = 1
        if color_count == 3:
            color_count = 2
            return COLOR_CODES["G"]
        return 0
    elif img.find_blobs(thre_yellow, pixels_threshold=200, merge=True):
        if last_color_label == "Y":
            color_count += 1
        else:
            last_color_label = "Y"
            color_count = 1
        if color_count == 3:
            color_count = 2
            return COLOR_CODES["Y"]
        return 0
    else:
        last_color_label = None
        color_count = 0
        return None

def main(model_addr=0x300000):
    sensor.reset()
    sensor.set_pixformat(sensor.RGB565)
    sensor.set_framesize(sensor.QVGA)
    sensor.set_windowing(sensor_window)
    sensor.set_auto_whitebal(False, rgb_gain_db = (64, 87, 246)) # ゲインを設定しておく

    sensor.set_brightness(0)            # 明度の調整
    sensor.set_saturation(0)            # 彩度の調整
    sensor.set_contrast(2)              # コントラストの調整
    global task
    task = kpu.load(model_addr)

    try:
        while(True):
            img = sensor.snapshot()
            if img is None or task is None:
                continue
            ch = detect_character_victim(img)
            col = detect_colored_victim(img)
            if ch:
                send(ch)
                print(ch)
            elif col:
                send(col)
                print(col)
            else:
                send(0)
                print(0)
            gc.collect()
    except Exception as e:
        print(e)
        raise e
    finally:
        kpu.deinit(task)


if __name__ == "__main__":
    try:
        main()
    except Exception as e:
        sys.print_exception(e)
    finally:
        gc.collect()
