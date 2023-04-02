import sys
import matplotlib.pyplot as plt
import numpy as np

# 打开文件
with open('../single/test_14/results/roboticArm.txt', 'r') as file1:
    lines_1 = file1.readlines()
with open('../single/test_14/results/gateController.txt', 'r') as file2:
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
        time1.append(item['time'] * 1000000) # 转化为以微秒为单位
        TT_delay.append(item['e2edelay'] * 1000000) # 转化为以微秒为单位
    else:
        time2.append(item['time'] * 1000000)
        BE_delay.append(item['e2edelay'] * 1000000)

# 读取GCL
# 交换机时隙大小：TT_interval
TT_interval = []
for item in gateController_table:
    time3.append(item['time'] * 1000000)
    TT_interval.append(item['TT-interval'] * 1000000) # 转化为以us为单位


# 计算延迟平均值，标准差，最大值，最小值
TT_avg = np.mean(TT_delay)
BE_avg = np.mean(BE_delay)
TT_std = np.std(TT_delay)
BE_std = np.std(BE_delay)
TT_max = max(TT_delay)
BE_max = max(BE_delay)
TT_min = min(TT_delay)
BE_min = min(BE_delay)
print("TT_avg: ", TT_avg, "ms    TT_std: " , TT_std, "ms    TT_max: ", TT_max, "ms    TT_min: ", TT_min,"ms")
print("BE_avg: ", BE_avg, "ms    BE_std: " , BE_std, "ms    BE_max: ", BE_max, "ms    BE_min: ", BE_min,"ms")

# 绘图设置
# 画两张图
fig, ax = plt.subplots(2, 1, figsize=(8, 6))
# 绘制横向网格线
ax[0].grid(axis="y", which='both',linestyle=':')
ax[1].grid(axis="y", which='both',linestyle=':')
# 绘制标题
ax[0].set_title("Traffic Delay", fontsize=18)
ax[1].set_title("TT Interval", fontsize=18)
# x轴设置
ax[0].set_xlabel('time (ms)', fontsize=18)
ax[1].set_xlabel('time (ms)', fontsize=18)
# y轴设置 从0~1，每各0.05一个刻度
y1, y2 = 0, 1050
dy_1 = 50
ax[0].set_yticks(np.arange(y1, y2, dy_1))
ax[0].set_ylabel('end2end delay(us)', fontsize=18)
y3, y4 = 0, 450
dy_2 = 50
ax[1].set_yticks(np.arange(y3, y4, dy_2))
ax[1].set_ylabel('TT Interval(us)', fontsize=18)
# 绘制曲线
ax[0].plot(time1, TT_delay, color='g',marker='.', linewidth=1, label='TT End2end Delay')
ax[0].plot(time2, BE_delay, color='r',marker='.',linewidth=1, label = "BE End2end Delay")
ax[1].plot(time3, TT_interval, color='b',marker='.', linewidth=1, label='Switch TT Interval')
# 标签设置
ax[0].legend(loc="upper right", fontsize=18)
ax[1].legend(loc="upper right", fontsize=18)
plt.show()