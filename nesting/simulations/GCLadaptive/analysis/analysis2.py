import sys
import matplotlib.pyplot as plt
import numpy as np

# 打开文件
with open('../twice/test_01/results/roboticArm.txt', 'r') as file1:
    lines_1 = file1.readlines()
with open('../twice/test_01/results/gateController_a.txt', 'r') as file2:
    lines_2 = file2.readlines()[1:]  # 跳过前两行数据
with open('../twice/test_01/results/gateController_b.txt', 'r') as file3:
    lines_3 = file3.readlines()[1:]  # 跳过前两行数据

# 读取数据
# 分别读取延迟，调整时隙大小，实际加载的交换机a的时隙大小，实际加载的交换机b的时隙大小
delay_table = []
interval_table = []
gateController_a_table = []
gateController_b_table = []

# 读取robotArm
for line in lines_1:
    line = line.strip('\n')
    dict = eval(line)
    if len(dict) == 5:
        delay_table.append(dict)
    else:
        interval_table.append(dict)
# 读取gateController_a
for line in lines_2:
    line = line.strip('\n')
    dict = eval(line)
    gateController_a_table.append(dict)
# 读取gateController_b
for line in lines_3:
    line = line.strip('\n')
    dict = eval(line)
    gateController_b_table.append(dict)

# 读取延迟
# 仿真时间：time1~4    TT延迟：TT_delay   BE延迟：BE_delay
time1 = []
time2 = []
time3 = []
time4 = []
TT_delay = []
BE_delay = []
for item in delay_table:
    if item['pcp'] == 7:
        time1.append(item['time'] * 1000) # 转化为以毫秒为单位
        TT_delay.append(item['e2edelay'] * 1000) # 转化为以毫秒为单位
    else:
        time2.append(item['time'] * 1000)
        BE_delay.append(item['e2edelay'] * 1000)

# 读取GCL
# 交换机a时隙大小：TT_interval_a    交换机b时隙大小：TT_interval_b
TT_interval_a = []
TT_interval_b = []
for item in gateController_a_table:
    time3.append(item['time'] * 1000)
    TT_interval_a.append(item['TT-interval'] * 1000) # 转化为以us为单位
for item in gateController_b_table:
    time4.append(item['time'] * 1000)
    TT_interval_b.append(item['TT-interval'] * 1000) # 转化为以us为单位


# 计算延迟平均值，标准差，最大值，最小值
TT_avg = np.mean(TT_delay)
BE_avg = np.mean(BE_delay)
TT_std = np.std(TT_delay)
BE_std = np.std(BE_delay)
TT_max = max(TT_delay)
BE_max = max(BE_delay)
TT_min = min(TT_delay)
BE_min = min(BE_delay)
print("TT_avg: " ,TT_avg, ", TT_std: " ,TT_std, " TT_max: ",TT_max, " TT_min: ", TT_min)
print("BE_avg: ", BE_avg, ", BE_std: " , BE_std, " BE_max: ", BE_max, " BE_min: ", BE_min)

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
y1, y2 = 0, 1.05
dy_1 = 0.05
ax[0].set_yticks(np.arange(y1, y2, dy_1))
ax[0].set_ylabel('end2end delay(ms)', fontsize=18)
y3, y4 = 0, 1.05
dy_2 = 0.05
ax[1].set_yticks(np.arange(y3, y4, dy_2))
ax[1].set_ylabel('TT Interval(ms)', fontsize=18)
# 绘制曲线
ax[0].plot(time1, TT_delay, color='g',marker='.', linewidth=1, label='TT End2end Delay')
ax[0].plot(time2, BE_delay, color='r',marker='.',linewidth=1, label = "BE End2end Delay")
ax[1].plot(time3, TT_interval_a, color='b',marker='.', linewidth=1, label='SwitchA TT Interval')
ax[1].plot(time4, TT_interval_b, color='y',marker='.',linewidth=1, label = "SwitchB TT Interval")
# 标签设置
ax[0].legend(loc="upper right", fontsize=18)
ax[1].legend(loc="upper right", fontsize=18)
plt.show()