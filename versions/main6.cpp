#include <iostream>
#include <vector>
#include <string>
#include <queue>
#include <cmath>
#include <fstream>
#include <algorithm>
#include <cstring>

using namespace std;

// 地图大小常量：100x100的网格
const int N = 100;
// 机器人数量：10个
const int ROBOT_NUM = 10;
// 船只数量：5艘
const int SHIP_NUM = 5;

// 货物结构体：存储货物的基本信息
struct Goods {
    int x, y;    // 货物在地图上的坐标
    int val;     // 货物的价值
};

// 机器人结构体：存储机器人的状态信息
struct Robot {
    int has_goods;       // 是否携带货物（0:无，1:有）
    int x, y;            // 机器人在地图上的当前坐标
    int status;          // 机器人的状态（0:不可用/损坏，其他:可用）
    int last_x = -1, last_y = -1;  // 上一帧的坐标，用于检测机器人是否卡住
    int stuck_count = 0;            // 卡住计数器，记录连续多少帧位置未变化
};

// 船只结构体：存储船只的状态信息
struct Ship {
    int status;      // 船只的状态
    int berth_id;    // 船只当前停靠的泊位ID
};

// 候选分配结构体：用于贪心算法中评估机器人-货物分配的优劣
struct Candidate {
    int robot_id;     // 机器人的ID（0-9）
    int good_idx;     // 货物在goods_list中的索引
    double score;     // 评分：价值/(距离+1)，用于贪心选择
    // 重载<运算符，实现按评分降序排序（评分高的排在前面）
    bool operator<(const Candidate& other) const {
        return score > other.score;
    }
};

// 全局变量定义
int frame_id, money;                    // 当前帧ID和当前拥有的金钱
char grid[N][N];                        // 地图网格，存储地图上的障碍物、泊位等信息
bool occupied[N][N];                    // 占用标记，用于机器人碰撞避免，记录每个位置是否有机器人
vector<Goods> goods_list;               // 当前地图上所有货物的列表
vector<Robot> robots(ROBOT_NUM);        // 所有机器人的列表
vector<Ship> ships(SHIP_NUM);           // 所有船只的列表
vector<pair<int, int>> berths;          // 所有泊位的坐标列表
int berth_dist[N][N];                   // 每个点到最近泊位的距离

// 方向数组：定义四个移动方向
// 0:右，1:左，2:上，3:下
int dx[] = {0, 0, -1, 1};
int dy[] = {1, -1, 0, 0};

// 计算每个点到最近泊位的距离（多源BFS）
void init_berth_dist() {
    memset(berth_dist, -1, sizeof(berth_dist));
    queue<pair<int, int>> q;
    for (auto& b : berths) {
        berth_dist[b.first][b.second] = 0;
        q.push(b);
    }

    while (!q.empty()) {
        auto curr = q.front();
        q.pop();
        int cx = curr.first;
        int cy = curr.second;

        for (int i = 0; i < 4; i++) {
            int nx = cx + dx[i];
            int ny = cy + dy[i];
            if (nx >= 0 && nx < N && ny >= 0 && ny < N &&
                grid[nx][ny] != '*' && grid[nx][ny] != '#' &&
                berth_dist[nx][ny] == -1) {
                berth_dist[nx][ny] = berth_dist[cx][cy] + 1;
                q.push({nx, ny});
            }
        }
    }
}

// 加载地图文件
// 从maps/map1.txt读取地图数据，并初始化泊位列表
void load_map() {
    ifstream in("maps/map1.txt");
    if (!in) {
        cerr << "地图加载失败!" << endl;
        return;
    }
    // 读取100x100的地图网格
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            in >> grid[i][j];
            // 如果该位置是泊位（标记为'B'），则记录其坐标
            if (grid[i][j] == 'B') {
                berths.push_back({i, j});
            }
        }
    }
    in.close();
}

// 读取每一帧的数据
// 从标准输入读取当前帧的游戏状态数据
// 返回值：成功读取返回true，读取失败（游戏结束）返回false
bool read_frame_data() {
    // 读取帧ID和当前金钱
    if (!(cin >> frame_id >> money)) return false;
    
    int k;
    cin >> k;  // 读取当前帧的货物数量
    // 读取所有货物的信息（坐标和价值）
    goods_list.resize(k);
    for (int i = 0; i < k; i++) {
        cin >> goods_list[i].x >> goods_list[i].y >> goods_list[i].val;
    }
    
    // 读取所有机器人的状态信息
    for (int i = 0; i < ROBOT_NUM; i++) {
        cin >> robots[i].has_goods >> robots[i].x >> robots[i].y >> robots[i].status;
    }
    
    // 读取所有船只的状态信息
    for (int i = 0; i < SHIP_NUM; i++) {
        cin >> ships[i].status >> ships[i].berth_id;
    }
    
    string s;
    cin >> s; // 读取 "OK" 确认标志，表示帧数据读取完成
    return true;
}

// 使用BFS（广度优先搜索）算法寻找从起点到目标位置的下一步移动方向
// 参数：
//   start_x, start_y: 起点坐标
//   target_x, target_y: 目标坐标
// 返回值：
//   0-3: 表示移动方向（右、左、上、下）
//   -1: 表示已在目标位置或无法到达目标
int bfs(int start_x, int start_y, int target_x, int target_y) {
    // 如果已经在目标位置，返回-1
    if (start_x == target_x && start_y == target_y) return -1;

    queue<pair<int, int>> q;
    q.push({start_x, start_y});
    
    // parent数组记录每个位置的父节点，用于回溯路径
    vector<vector<pair<int, int>>> parent(N, vector<pair<int, int>>(N, {-1, -1}));
    // visited数组记录每个位置是否已被访问
    vector<vector<bool>> visited(N, vector<bool>(N, false));
    
    visited[start_x][start_y] = true;
    
    bool found = false;
    // BFS搜索主循环
    while (!q.empty()) {
        auto curr = q.front();
        q.pop();
        int cx = curr.first;
        int cy = curr.second;

        // 找到目标位置
        if (cx == target_x && cy == target_y) {
            found = true;
            break;
        }

        // 尝试向四个方向移动
        for (int i = 0; i < 4; i++) {
            int nx = cx + dx[i];
            int ny = cy + dy[i];

            // 检查边界和是否已访问
            if (nx >= 0 && nx < N && ny >= 0 && ny < N && !visited[nx][ny]) {
                // 检查障碍物和动态占用情况
                // '*'和'#'表示障碍物，occupied表示有其他机器人占用
                if (grid[nx][ny] != '*' && grid[nx][ny] != '#' && !occupied[nx][ny]) {
                    visited[nx][ny] = true;
                    parent[nx][ny] = {cx, cy};
                    q.push({nx, ny});
                }
            }
        }
    }

    // 如果未找到路径，返回-1
    if (!found) return -1;

    // 从目标位置回溯到起点，找到第一步的移动方向
    int curr_x = target_x;
    int curr_y = target_y;
    while (true) {
        pair<int, int> p = parent[curr_x][curr_y];
        // 如果父节点是起点，说明找到了紧邻起点的下一步位置
        if (p.first == start_x && p.second == start_y) {
            // 确定移动方向
            for (int i = 0; i < 4; i++) {
                if (start_x + dx[i] == curr_x && start_y + dy[i] == curr_y) {
                    return i;
                }
            }
        }
        curr_x = p.first;
        curr_y = p.second;
    }
    return -1;
}

int main() {
    // 加载地图数据
    load_map();
    init_berth_dist(); // 预计算泊位距离场
    
    // 主循环：处理每一帧的游戏数据
    while (read_frame_data()) {
        // 初始化占用地图，标记当前所有机器人的位置
        memset(occupied, 0, sizeof(occupied));
        for(int i=0; i<ROBOT_NUM; i++) {
            occupied[robots[i].x][robots[i].y] = true;
        }

        // ========== 货物的全局分配阶段 ==========
        // 使用贪心算法为空闲的机器人分配货物
        vector<int> robot_target_good(ROBOT_NUM, -1);  // 记录每个机器人的目标货物索引，-1表示无目标
        vector<bool> good_assigned(goods_list.size(), false);  // 记录货物是否已被分配
        vector<Candidate> candidates;  // 候选分配列表

        // 为每个空闲且未携带货物的机器人计算所有货物的评分
        for (int i = 0; i < ROBOT_NUM; i++) {
            // 跳过不可用的机器人和已携带货物的机器人
            if (robots[i].status == 0 || robots[i].has_goods) continue;

            // 计算该机器人到每个货物的评分
            for (int j = 0; j < goods_list.size(); j++) {
                // 计算曼哈顿距离（|x1-x2| + |y1-y2|）
                int d = abs(robots[i].x - goods_list[j].x) + abs(robots[i].y - goods_list[j].y);
                
                // 获取货物到最近泊位的真实距离
                int dist_to_berth = berth_dist[goods_list[j].x][goods_list[j].y];
                if (dist_to_berth == -1) continue; // 无法到达泊位的货物忽略

                // 计算评分：货物价值 / (人货距离 + 货到泊位距离 + 1)
                double score = (double)goods_list[j].val / (d + dist_to_berth + 1.0);
                candidates.push_back({i, j, score});
            }
        }
        
        // 按评分降序排序（评分高的优先分配）
        sort(candidates.begin(), candidates.end());
        
        // 贪心分配：按评分从高到低依次分配
        // 确保每个机器人只分配一个货物，每个货物只分配给一个机器人
        for (const auto& cand : candidates) {
            if (robot_target_good[cand.robot_id] == -1 && !good_assigned[cand.good_idx]) {
                robot_target_good[cand.robot_id] = cand.good_idx;
                good_assigned[cand.good_idx] = true;
            }
        }

        // ========== 优先级计算与排序 ==========
        // 根据货物价值分配优先级，携带货物的优先级最高
        vector<int> robot_priority(ROBOT_NUM, 0);
        vector<int> p_order(ROBOT_NUM);
        for (int i = 0; i < ROBOT_NUM; i++) {
            p_order[i] = i;
            if (robots[i].has_goods) {
                robot_priority[i] = 10000; // 携带货物的优先级最高
            } else if (robot_target_good[i] != -1) {
                robot_priority[i] = goods_list[robot_target_good[i]].val;
            } else {
                robot_priority[i] = 0;
            }
        }
        // 按优先级降序排序，优先级高的机器人先行动
        sort(p_order.begin(), p_order.end(), [&](int a, int b){
            return robot_priority[a] > robot_priority[b];
        });

        // ========== 机器人处理阶段 ==========
        for (int k = 0; k < ROBOT_NUM; k++) {
            int i = p_order[k]; // 按优先级顺序处理机器人

            // 跳过不可用的机器人
            if (robots[i].status == 0) continue; 

            // 更新卡死状态检测
            // 如果机器人位置与上一帧相同，说明可能卡住了
            if (robots[i].x == robots[i].last_x && robots[i].y == robots[i].last_y) {
                robots[i].stuck_count++;  // 增加卡住计数
            } else {
                robots[i].stuck_count = 0;  // 重置卡住计数
            }
            // 更新上一帧位置
            robots[i].last_x = robots[i].x;
            robots[i].last_y = robots[i].y;

            // 临时释放当前位置，用于路径规划
            // 这样机器人可以规划从当前位置出发的路径
            occupied[robots[i].x][robots[i].y] = false;

            int move_dir = -1;  // 移动方向，-1表示不移动
            bool action_taken = false;  // 是否执行了动作（get或pull）
            int tx = -1, ty = -1; // 目标坐标，用于防卡死时的启发式选择

            if (robots[i].has_goods) {
                // ========== 携带货物状态：前往最近的泊位 ==========
                int best_dist = 1e9;  // 最小距离初始化为很大值
                int target_x = -1, target_y = -1;
                
                // 遍历所有泊位，找到距离当前机器人最近的泊位
                for (auto& b : berths) {
                    int d = abs(robots[i].x - b.first) + abs(robots[i].y - b.second);
                    if (d < best_dist) {
                        best_dist = d;
                        target_x = b.first;
                        target_y = b.second;
                    }
                }

                if (target_x != -1) {
                    tx = target_x; ty = target_y; // 记录目标
                    // 如果已经在泊位位置，执行pull操作（将货物放到船上）
                    if (robots[i].x == target_x && robots[i].y == target_y) {
                        cout << "pull " << i << endl;
                        action_taken = true;
                    } else {
                        // 否则使用BFS规划路径前往泊位
                        move_dir = bfs(robots[i].x, robots[i].y, target_x, target_y);
                    }
                }
            } else {
                // ========== 未携带货物状态：前往预分配的目标货物 ==========
                int target_idx = robot_target_good[i];
                if (target_idx != -1) {
                    tx = goods_list[target_idx].x; ty = goods_list[target_idx].y; // 记录目标
                    // 如果已经在货物位置，执行get操作（捡起货物）
                    if (robots[i].x == goods_list[target_idx].x && robots[i].y == goods_list[target_idx].y) {
                        cout << "get " << i << endl;
                        action_taken = true;
                    } else {
                        // 否则使用BFS规划路径前往货物
                        move_dir = bfs(robots[i].x, robots[i].y, goods_list[target_idx].x, goods_list[target_idx].y);
                    }
                }
            }

            // ========== 处理移动 ==========
            if (!action_taken) {
                int final_move_dir = -1;

                // 1. 尝试最优路径
                if (move_dir != -1) {
                    int nx = robots[i].x + dx[move_dir];
                    int ny = robots[i].y + dy[move_dir];
                    // 检查是否被占用
                    if (!occupied[nx][ny]) {
                        final_move_dir = move_dir;
                    }
                }

                // 2. 防卡死机制：如果最优路径被阻挡 或 无路径，且已卡住一段时间
                // 阈值设为5，意味着如果连续5帧没动，就开始尝试绕路
                if (final_move_dir == -1 && robots[i].stuck_count >5) {
                    vector<int> alt_dirs;
                    for (int d = 0; d < 4; d++) {
                        if (d == move_dir) continue; // 跳过原本想走但走不通的方向
                        
                        int nx = robots[i].x + dx[d];
                        int ny = robots[i].y + dy[d];
                        
                        // 检查合法性：边界、障碍物、是否被占用
                        if (nx >= 0 && nx < N && ny >= 0 && ny < N && 
                            grid[nx][ny] != '*' && grid[nx][ny] != '#' && !occupied[nx][ny]) {
                            alt_dirs.push_back(d);
                        }
                    }

                    if (!alt_dirs.empty()) {
                        // 如果有明确目标，按曼哈顿距离排序，优先选择离目标近的
                        if (tx != -1) {
                            sort(alt_dirs.begin(), alt_dirs.end(), [&](int a, int b){
                                int da = abs(robots[i].x + dx[a] - tx) + abs(robots[i].y + dy[a] - ty);
                                int db = abs(robots[i].x + dx[b] - tx) + abs(robots[i].y + dy[b] - ty);
                                return da < db;
                            });
                        }
                        final_move_dir = alt_dirs[0];
                    }
                }

                if (final_move_dir != -1) {
                    cout << "move " << i << " " << final_move_dir << endl;
                    int nx = robots[i].x + dx[final_move_dir];
                    int ny = robots[i].y + dy[final_move_dir];
                    occupied[nx][ny] = true;
                } else {
                    occupied[robots[i].x][robots[i].y] = true; // 保持原地
                }
            } else {
                // 执行了get或pull动作，机器人保持原地不动
                occupied[robots[i].x][robots[i].y] = true;
            }
        }

        // ========== 船只处理阶段 ==========
        // 简单策略：让所有船只都执行go命令（离开泊位）
        for (int i = 0; i < SHIP_NUM; i++) {
            cout << "go " << i << endl;
        }

        // 输出帧结束标志，表示本帧的所有指令已输出完毕
        cout << "OK" << endl;
    }
    return 0;
}
