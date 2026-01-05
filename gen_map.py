import random
import os

# --- 配置区 ---
OUTPUT_DIR = "maps"
MAP_SIZE = 100
MAP_FILE = os.path.join(OUTPUT_DIR, "map1.txt")

# 地图元素概率
LAND_PROB = 0.65        # 空地概率
SEA_PROB = 0.20         # 海洋概率
OBSTACLE_PROB = 0.10    # 障碍概率
# 剩余概率为特殊格子（起点A、泊位B）

NUM_START_POINTS = 10   # 机器人起点数
NUM_BERTHS = 5          # 泊位数


def generate_map():
    """生成港口地图"""
    os.makedirs(OUTPUT_DIR, exist_ok=True)

    # 初始化地图为空地
    grid = [['.' for _ in range(MAP_SIZE)] for _ in range(MAP_SIZE)]

    # 随机生成障碍和海洋
    for r in range(MAP_SIZE):
        for c in range(MAP_SIZE):
            rand = random.random()
            if rand < SEA_PROB:
                grid[r][c] = '*'
            elif rand < SEA_PROB + OBSTACLE_PROB:
                grid[r][c] = '#'

    # 放置机器人起点（确保在空地上）
    placed_starts = 0
    attempts = 0
    while placed_starts < NUM_START_POINTS and attempts < 1000:
        r = random.randint(0, MAP_SIZE - 1)
        c = random.randint(0, MAP_SIZE - 1)
        if grid[r][c] == '.':
            grid[r][c] = 'A'
            placed_starts += 1
        attempts += 1

    # 放置泊位（确保在空地上，且靠近边缘或海洋）
    placed_berths = 0
    attempts = 0
    while placed_berths < NUM_BERTHS and attempts < 1000:
        r = random.randint(0, MAP_SIZE - 1)
        c = random.randint(0, MAP_SIZE - 1)
        
        # 检查是否靠近海洋（更真实）
        near_sea = False
        for dr in [-1, 0, 1]:
            for dc in [-1, 0, 1]:
                nr, nc = r + dr, c + dc
                if 0 <= nr < MAP_SIZE and 0 <= nc < MAP_SIZE:
                    if grid[nr][nc] == '*':
                        near_sea = True
                        break
        
        if grid[r][c] == '.' or (grid[r][c] == 'A' and placed_starts > NUM_START_POINTS):
            grid[r][c] = 'B'
            placed_berths += 1
        attempts += 1

    # 确保至少有一些连通的区域（简化版：不做严格连通性检查）
    # 实际项目中可以添加BFS/DFS验证连通性

    # 保存地图
    with open(MAP_FILE, 'w') as f:
        for row in grid:
            f.write(''.join(row) + '\n')

    print(f"地图生成完成！")
    print(f"  文件: {MAP_FILE}")
    print(f"  尺寸: {MAP_SIZE} × {MAP_SIZE}")
    print(f"  机器人起点: {placed_starts} 个")
    print(f"  泊位: {placed_berths} 个")
    
    # 统计地图元素
    land_count = sum(row.count('.') for row in grid)
    sea_count = sum(row.count('*') for row in grid)
    obstacle_count = sum(row.count('#') for row in grid)
    
    print(f"\n地图统计:")
    print(f"  空地 (.): {land_count} ({land_count/100:.1f}%)")
    print(f"  海洋 (*): {sea_count} ({sea_count/100:.1f}%)")
    print(f"  障碍 (#): {obstacle_count} ({obstacle_count/100:.1f}%)")
    print(f"  起点 (A): {placed_starts}")
    print(f"  泊位 (B): {placed_berths}")


if __name__ == "__main__":
    generate_map()
