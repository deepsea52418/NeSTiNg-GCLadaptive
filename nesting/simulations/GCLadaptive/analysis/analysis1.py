import sys
import matplotlib.pyplot as plt
import numpy as np

# 打开文件
with open('../single/test_12/results/roboticArm.txt', 'r') as file1:
    lines_1 = file1.readlines()
with open('../single/test_12/results/gateController.txt', 'r') as file2:
    lines_2 = file2.readlines()[2:]  # 跳过前两行数据


# 读取数据
# 分别读取延迟，调整时隙大小，实际加载的交换机a的时隙大小，实际加载的交换机b的时隙大小
delay_table = []
interval_table = []
gateController_table = []


# 读取robotArm
for line in lines_1:
    line = line.strip('\n')
    dict = eval(line)
    if len(dict) == 5:
        delay_table.append(dict)
    else:
        interval_table.append(dict)
# 读取gateController
for line in lines_2:
    line = line.strip('\n')
    dict = eval(line)
    gateController_table.append(dict)


# 读取延迟
# 仿真时间：time1~3    TT延迟：TT_delay   BE延迟：BE_delay
time1 = []
time2 = []
time3 = []
TT_delay = []
BE_delay = []
for item in delay_table:
    if item['pcp'] == 7:
        time1.append(item['time'] * 1000) # ms
        TT_delay.append(item['e2edelay'] * 1000000) # us
    else:
        time2.append(item['time'] * 1000)
        BE_delay.append(item['e2edelay'] * 1000000)

# 读取GCL
# 交换机时隙大小：TT_interval
TT_interval = []
for item in gateController_table:
    time3.append(item['time'] * 1000)
    TT_interval.append(item['TT-interval'] * 1000000) # 转化为以us为单位

# 读取RobotArm文件中的时隙大小
time4 = []
interval = []
for item in interval_table:
    time4.append(item['time'] * 1000) # ms
    interval.append(item['pcp=7-interval'] * 1000000)



# 计算延迟平均值，标准差，最大值，最小值
TT_avg = np.mean(TT_delay)
BE_avg = np.mean(BE_delay)
TT_std = np.std(TT_delay)
BE_std = np.std(BE_delay)
TT_max = max(TT_delay)
BE_max = max(BE_delay)
TT_min = min(TT_delay)
BE_min = min(BE_delay)
print("TT_avg: ", round(TT_avg,2), "us    TT_std: " , round(TT_std,2), "us    TT_max: ", round(TT_max,2), "us    TT_min: ", round(TT_min,2),"us")
print("BE_avg: ", round(BE_avg,2), "us    BE_std: " , round(BE_std,2), "us    BE_max: ", round(BE_max,2), "us    BE_min: ", round(BE_min,2),"us")
# 绘图设置
# 画两张图
fig, ax = plt.subplots(2, 1, figsize=(22.5, 45))
# 绘制横向网格线
ax[0].grid(axis="y", which='both',linestyle=':')
ax[1].grid(axis="y", which='both',linestyle=':')
# 绘制标题
ax[0].set_title("Traffic Delay", fontsize=30)
ax[1].set_title("TT Interval", fontsize=30)
# x轴设置
x1, x2 = -0.01, 140
dx = 10
ax[0].set_xticks(np.arange(x1, x2, dx))
x_tick_labels = ax[0].get_xticklabels()
for label in x_tick_labels:
    label.set_fontsize(18)
ax[1].set_xticks(np.arange(x1, x2, dx))
x_tick_labels = ax[1].get_xticklabels()
for label in x_tick_labels:
    label.set_fontsize(18)
# y轴设置
y1, y2 = 0, 1100
dy_1 = 100
ax[0].set_yticks(np.arange(y1, y2, dy_1))
y_tick_labels = ax[0].get_yticklabels()
for label in y_tick_labels:
    label.set_fontsize(18)
ax[0].set_ylabel('end2end delay(us)', fontsize=22)
y3, y4 = 0, 450
dy_2 = 50
ax[1].set_yticks(np.arange(y3, y4, dy_2))
y_tick_labels = ax[1].get_yticklabels()
for label in y_tick_labels:
    label.set_fontsize(18)
ax[1].set_ylabel('TT Interval(us)', fontsize=22)
# 绘制曲线
ax[0].plot(time1, TT_delay, color='g',marker='.', linewidth=1, label='TT End2end Delay')
ax[0].plot(time2, BE_delay, color='r',marker='.',linewidth=1, label = "BE End2end Delay")
ax[1].plot(time3, TT_interval, color='b',marker='.', linewidth=1, label='Switch TT Interval')
#ax[1].plot(time4, interval, color='y',marker='.', linewidth=1, label='GCL TT Interval')
# 标签设置
ax[0].legend(loc="upper right", fontsize=25)
ax[1].legend(loc="upper right", fontsize=25)
ax[0].set_xlim(0, 40)
ax[0].set_ylim(0, 1000)
ax[1].set_xlim(0, 40)
ax[1].set_ylim(0, 400)
# 设置子图横坐标
ax[0].text(0.99, -0.1, 'time (ms)', fontsize=22, ha='right', va='bottom', transform=ax[0].transAxes)
ax[1].text(0.99, -0.1, 'time (ms)', fontsize=22, ha='right', va='bottom', transform=ax[1].transAxes)
# 裁剪图片
plt.subplots_adjust(left=0.055, right=0.99, top=0.95, bottom=0.05)
plt.show()