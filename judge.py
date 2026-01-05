import sys
import subprocess
import random
import os
import time
import platform

# --- 配置区 ---
MAP_FILE = "maps/map1.txt"
MAX_FRAMES = 1000
ROBOT_COUNT = 10
SHIP_COUNT = 5
MAP_SIZE = 100

# 自动判断可执行文件名称
if platform.system() == "Windows":
    STUDENT_CMD = ["main.exe"]
else:
    STUDENT_CMD = ["./main"]  # Linux / MacOS


class GameState:
    def __init__(self, map_data):
        self.map = map_data
        self.money = 0
        self.frame = 1
        self.goods = {}
        self.robots = []
        self.ships = [{"capacity": 0} for _ in range(SHIP_COUNT)]
        self._init_robots()

    def _init_robots(self):
        starts = []
        for r in range(MAP_SIZE):
            for c in range(MAP_SIZE):
                if self.map[r][c] == 'A': starts.append((r, c))
        while len(starts) < ROBOT_COUNT: starts.append((0, 0))
        for i in range(ROBOT_COUNT):
            self.robots.append({"x": starts[i][0], "y": starts[i][1], "goods": 0, "status": 1})

    def step_goods(self):
        if len(self.goods) < 50 and random.random() < 0.2:
            x, y = random.randint(0, MAP_SIZE - 1), random.randint(0, MAP_SIZE - 1)
            if self.map[x][y] == '.' and (x, y) not in self.goods:
                self.goods[(x, y)] = {'val': random.randint(10, 100), 'expire': self.frame + 1000}
        expired = [k for k, v in self.goods.items() if v['expire'] <= self.frame]
        for k in expired: del self.goods[k]

    def get_input_str(self):
        lines = [f"{self.frame} {self.money}"]
        lines.append(f"{len(self.goods)}")
        for (x, y), v in self.goods.items():
            lines.append(f"{x} {y} {v['val']}")
        for r in self.robots:
            lines.append(f"{1 if r['goods'] > 0 else 0} {r['x']} {r['y']} {r['status']}")
        for i in range(SHIP_COUNT):
            lines.append(f"1 {i}")
        lines.append("OK")
        return "\n".join(lines) + "\n"


def run_game():
    # 0. 检查地图
    if not os.path.exists(MAP_FILE):
        print(f"Error: {MAP_FILE} not found. Run gen_map.py first.")
        return

    # 1. 检查C++可执行文件
    exe_path = STUDENT_CMD[0]
    if not os.path.exists(exe_path):
        print(f"\n[错误] 找不到可执行文件: {exe_path}")
        print("请先编译你的 C++ 代码！")
        print(f"Windows: g++ main.cpp -o main.exe")
        print(f"Linux/Mac: g++ main.cpp -o main")
        return

    with open(MAP_FILE) as f:
        map_data = [list(line.strip()) for line in f]

    game = GameState(map_data)

    # 2. 启动子进程
    print(f"Starting Process: {STUDENT_CMD[0]}")
    try:
        proc = subprocess.Popen(STUDENT_CMD, stdin=subprocess.PIPE, stdout=subprocess.PIPE, text=True, bufsize=1)
    except Exception as e:
        print(f"Failed to start student program: {e}")
        return

    print(f"--- Simulation Start (Total Frames: {MAX_FRAMES}) ---")

    try:
        for frame in range(1, MAX_FRAMES + 1):
            game.frame = frame
            game.step_goods()

            # 发送数据
            try:
                proc.stdin.write(game.get_input_str())
                proc.stdin.flush()
            except BrokenPipeError:
                print("Error: Student program exited unexpectedly.")
                break

            # 接收指令
            commands = []
            while True:
                line = proc.stdout.readline().strip()
                if not line or line == "OK": break
                commands.append(line)

            # 处理逻辑 (与Python版一致)
            next_pos = {}
            for cmd in commands:
                parts = cmd.split()
                if not parts: continue
                if parts[0] == "move":
                    rid, d = int(parts[1]), int(parts[2])
                    r = game.robots[rid]
                    dx, dy = {0: (0, 1), 1: (0, -1), 2: (-1, 0), 3: (1, 0)}.get(d, (0, 0))
                    nx, ny = r['x'] + dx, r['y'] + dy
                    if 0 <= nx < MAP_SIZE and 0 <= ny < MAP_SIZE and game.map[nx][ny] not in ['#', '*']:
                        next_pos[rid] = (nx, ny)

            current_occupied = set((r['x'], r['y']) for r in game.robots)
            for rid, (nx, ny) in next_pos.items():
                if (nx, ny) not in current_occupied:
                    dx = nx - game.robots[rid]['x']
                    dy = ny - game.robots[rid]['y']
                    current_occupied.remove((game.robots[rid]['x'], game.robots[rid]['y']))
                    game.robots[rid]['x'], game.robots[rid]['y'] = nx, ny
                    current_occupied.add((nx, ny))

            for cmd in commands:
                parts = cmd.split()
                if not parts: continue
                action = parts[0]
                if action == "get":
                    rid = int(parts[1])
                    r = game.robots[rid]
                    if r['goods'] == 0 and (r['x'], r['y']) in game.goods:
                        val = game.goods.pop((r['x'], r['y']))['val']
                        r['goods'] = val
                elif action == "pull":
                    rid = int(parts[1])
                    r = game.robots[rid]
                    if r['goods'] > 0 and game.map[r['x']][r['y']] == 'B':
                        game.ships[0]['capacity'] += r['goods']
                        r['goods'] = 0
                elif action == "go":
                    sid = int(parts[1])
                    profit = game.ships[sid]['capacity']
                    game.money += profit
                    game.ships[sid]['capacity'] = 0

            if frame % 100 == 0:
                print(f"Frame {frame}: Money = {game.money}")

    except Exception as e:
        print(f"Runtime Error: {e}")
    finally:
        proc.terminate()
        print(f"--- Game Over ---")
        print(f"Final Score: {game.money}")


if __name__ == "__main__":
    run_game()