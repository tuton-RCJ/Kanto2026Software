import sensor, image, time
import KPU as kpu
import gc, sys
from machine import UART
import ustruct

last_character_label = None
last_color_label = None
character_count = 0
color_count = 0
min_confidence = 0.7
thre_green = [(30,100, -60, -25, -5, 20)]
thre_red = [(20, 75, 40, 60, 30, 60)]
thre_yellow = [(25, 100, -10, 10, 30, 70)]
task = None
uart = UART(UART.UART1, 115200,8,0,0, timeout=1000, read_buf_len=4096)
labels = ["H", "None", "S", "U"]

gc.enable()
gc.threshold(gc.mem_alloc() + gc.mem_free())

def send(data):
    uart.write(ustruct.pack("B", data))
    uart.flush()

def detect_character_victim(img):
    global last_character_label
    global character_count
    global task
    labels = ["H", "None", "S", "U"]

    fmap = kpu.forward(task, img)
    plist=fmap[:]
    pmax=max(plist)
    max_index=plist.index(pmax)
    label = labels[max_index].strip()
    img.draw_string(0,0, "%.2f : %s" %(pmax, label), scale=2, color=(0, 0, 0))

    if pmax > min_confidence:
        if last_character_label == label:
            character_count += 1
        else:
            last_character_label = label
            character_count = 1

        if character_count == 3:
            character_count = 2
            if label == "H":
                max_index = 1
            return max_index
    else:
        last_character_label = None
        character_count = 0
def detect_colored_victim(img):
    global last_color_label
    global color_count
    if img.find_blobs(thre_red, pixels_threshold=200, area_threshold=200, merge=True):
        if last_color_label == "R":
            color_count += 1
        else:
            last_color_label = "R"
            color_count = 1
        if color_count == 3:
            color_count = 2
        return 4
    elif img.find_blobs(thre_green, pixels_threshold=200, area_threshold=200, merge=True):
        if last_color_label == "G":
            color_count += 1
        else:
            last_color_label = "G"
            color_count = 1
        if color_count == 3:
            color_count = 2
        return 5
    elif img.find_blobs(thre_yellow, pixels_threshold=200, area_threshold=200, merge=True):
        if last_color_label == "Y":
            color_count += 1
        else:
            last_color_label = "Y"
            color_count = 1
        if color_count == 3:
            color_count = 2
    else:
        last_color_label = None
        color_count = 0
        return 6

def main(model_addr=0x300000, sensor_window=(224, 224)):
    global task
    sensor.reset()
    sensor.set_pixformat(sensor.RGB565)
    sensor.set_framesize(sensor.QVGA)
    sensor.set_windowing(sensor_window)
    sensor.set_auto_exposure(False, 10000)
    sensor.sleep(2000)
    sensor.run(1)
    img = image.Image(size=(320, 240))

    task = kpu.load(model_addr)

    try:
        while(True):
            img = sensor.snapshot()
            if img is None or task is None:
                continue
            ch = detect_character_victim(img)
            col = detect_colored_victim(img)
            if ch is not None:
                send(ch)
            if col is not None:
                send(col)
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
