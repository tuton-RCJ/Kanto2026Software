import sensor, image, time

thre_green = [(20,70, -40, -20, 5, 20)]
thre_red = [(30, 60, 55, 80, 40, 70)]
thre_yellow = [(35, 100, 0, 30, 50, 75)]
green = (0, 255, 0)
red = (255, 0, 0)
yellow = (255, 255, 0)

sensor.reset()
sensor.set_pixformat(sensor.RGB565)
sensor.set_framesize(sensor.QQVGA)
sensor.skip_frames(time=2000)

sensor.set_auto_gain(False, gain_db=20)
sensor.set_auto_exposure(False, exposure_us=8000)
sensor.skip_frames(time=1500)

while True:
    img = sensor.snapshot()
    stat = img.get_statistics(roi=(39,66,24,26))
    print(stat.l_mean(),stat.a_mean(), stat.b_mean(), stat.l_stdev(), stat.a_stdev(), stat.b_stdev())
    for b in img.find_blobs(thre_yellow, merge=True, margin=3):
        img.draw_rectangle(b[:4],color=yellow)
    for b in img.find_blobs(thre_red, merge=True, margin=3):
        img.draw_rectangle(b[:4], color=red)
    for b in img.find_blobs(thre_green, merge=True, margin=3):
        img.draw_rectangle(b[:4], color=green)
    print(sensor.get_rgb_gain_db())
