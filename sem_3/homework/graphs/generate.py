import matplotlib.pyplot as plt
from matplotlib import rcParams

plt.style.use('dark_background')
rcParams['font.size'] = 14

methods = ['SHM', 'MSG', 'FIFO']

data = {
    '8KB': [0.000326, 0.000363, 0.000201],
    '4MB': [0.011860, 0.008808, 0.011780],
    '2GB': [2.082411, 3.004880, 3.104620]
}

colors = ['#FF6B6B', '#4ECDC4', '#45B7D1']

for file_size, times in data.items():
    fig, ax = plt.subplots(figsize=(10, 6))

    bars = ax.bar(methods, times, color=colors, width=0.6,
                  edgecolor='white', linewidth=1, alpha=0.8)

    ax.set_xlabel('IPC методы', fontsize=16, fontweight='bold')
    ax.set_ylabel('Время выполнения (секунды)', fontsize=16, fontweight='bold')
    ax.set_title(f'Производительность IPC методов (Файл {file_size})',
                 fontsize=18, fontweight='bold', pad=20)
    ax.grid(True, alpha=0.3)

    plt.tight_layout()
    plt.savefig(f'ipc_{file_size}.png', dpi=150, bbox_inches='tight')
    plt.show()
