import matplotlib.pyplot as plt
from matplotlib import rcParams

plt.style.use('dark_background')
rcParams['figure.figsize'] = 12, 6
rcParams['font.size'] = 14

threads         = [1, 4, 9, 16]
execution_times = [0.355299, 0.278737, 0.179058, 0.118607]  

colors = ['#FF6B6B', '#4ECDC4', '#45B7D1', '#96CEB4']

fig, ax = plt.subplots(figsize=(14, 8))

bars = ax.bar(threads, execution_times, color=colors, edgecolor='white', linewidth=2, alpha=0.8)
ax.set_xlabel('Количество потоков', fontsize=16, fontweight='bold')
ax.set_ylabel('Время выполнения (с)', fontsize=16, fontweight='bold')
ax.set_title('Зависимость времени выполнения от количества потоков', 
             fontsize=18, fontweight='bold', pad=20)
ax.grid(True, alpha=0.3)
ax.set_xticks(threads)

for bar, time in zip(bars, execution_times):
    height = bar.get_height()
    ax.text(bar.get_x() + bar.get_width()/2., height + 0.005,
            f'{time}с', ha='center', va='bottom', fontweight='bold', fontsize=12)

plt.savefig('graph.png')

