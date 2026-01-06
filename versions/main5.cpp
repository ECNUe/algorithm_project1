#include <iostream>
#include <vector>
#include <string>
#include <queue>
#include <cmath>
#include <fstream>
#include <algorithm>
#include <cstring>

using namespace std;

const int N = 100;
const int ROBOT_NUM = 10;
const int SHIP_NUM = 5;

struct Goods {
    int x, y;
    int val;
};

struct Robot {
    int has_goods;
    int x, y;
    int status;
    int last_x = -1, last_y = -1;
    int stuck_count = 0;
};

struct Ship {
    int status;
    int berth_id;
};

struct Candidate {
    int robot_id;
    int good_idx;
    double score;
    bool operator<(const Candidate& other) const {
        return score > other.score;
    }
};

int frame_id, money;
char grid[N][N];
bool occupied[N][N];
vector<Goods> goods_list;
vector<Robot> robots(ROBOT_NUM);
vector<Ship> ships(SHIP_NUM);
vector<pair<int, int>> berths;
int berth_dist[N][N];

int dx[] = {0, 0, -1, 1};
int dy[] = {1, -1, 0, 0};

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

void load_map() {
    ifstream in("maps/map1.txt");
    if (!in) {
        cerr << "地图加载失败!" << endl;
        return;
    }
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            in >> grid[i][j];
            if (grid[i][j] == 'B') {
                berths.push_back({i, j});
            }
        }
    }
    in.close();
}

bool read_frame_data() {
    if (!(cin >> frame_id >> money)) return false;
    int k;
    cin >> k;
    goods_list.resize(k);
    for (int i = 0; i < k; i++) {
        cin >> goods_list[i].x >> goods_list[i].y >> goods_list[i].val;
    }
    for (int i = 0; i < ROBOT_NUM; i++) {
        cin >> robots[i].has_goods >> robots[i].x >> robots[i].y >> robots[i].status;
    }
    for (int i = 0; i < SHIP_NUM; i++) {
        cin >> ships[i].status >> ships[i].berth_id;
    }
    string s;
    cin >> s;
    return true;
}

int bfs(int start_x, int start_y, int target_x, int target_y) {
    if (start_x == target_x && start_y == target_y) return -1;

    queue<pair<int, int>> q;
    q.push({start_x, start_y});
    
    vector<vector<pair<int, int>>> parent(N, vector<pair<int, int>>(N, {-1, -1}));
    vector<vector<bool>> visited(N, vector<bool>(N, false));
    
    visited[start_x][start_y] = true;
    
    bool found = false;
    while (!q.empty()) {
        auto curr = q.front();
        q.pop();
        int cx = curr.first;
        int cy = curr.second;

        if (cx == target_x && cy == target_y) {
            found = true;
            break;
        }

        for (int i = 0; i < 4; i++) {
            int nx = cx + dx[i];
            int ny = cy + dy[i];

            if (nx >= 0 && nx < N && ny >= 0 && ny < N && !visited[nx][ny]) {
                if (grid[nx][ny] != '*' && grid[nx][ny] != '#' && !occupied[nx][ny]) {
                    visited[nx][ny] = true;
                    parent[nx][ny] = {cx, cy};
                    q.push({nx, ny});
                }
            }
        }
    }

    if (!found) return -1;

    int curr_x = target_x;
    int curr_y = target_y;
    while (true) {
        pair<int, int> p = parent[curr_x][curr_y];
        if (p.first == start_x && p.second == start_y) {
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
    load_map();
    init_berth_dist();
    
    while (read_frame_data()) {
        memset(occupied, 0, sizeof(occupied));
        for(int i=0; i<ROBOT_NUM; i++) {
            occupied[robots[i].x][robots[i].y] = true;
        }

        vector<int> robot_target_good(ROBOT_NUM, -1);
        vector<bool> good_assigned(goods_list.size(), false);
        vector<Candidate> candidates;

        for (int i = 0; i < ROBOT_NUM; i++) {
            if (robots[i].status == 0 || robots[i].has_goods) continue;

            for (int j = 0; j < goods_list.size(); j++) {
                int d = abs(robots[i].x - goods_list[j].x) + abs(robots[i].y - goods_list[j].y);
                int dist_to_berth = berth_dist[goods_list[j].x][goods_list[j].y];
                if (dist_to_berth == -1) continue;

                double score = (double)goods_list[j].val / (d + dist_to_berth + 1.0);
                candidates.push_back({i, j, score});
            }
        }
        
        sort(candidates.begin(), candidates.end());
        
        for (const auto& cand : candidates) {
            if (robot_target_good[cand.robot_id] == -1 && !good_assigned[cand.good_idx]) {
                robot_target_good[cand.robot_id] = cand.good_idx;
                good_assigned[cand.good_idx] = true;
            }
        }

        // Removed priority sorting here

        for (int i = 0; i < ROBOT_NUM; i++) {
            if (robots[i].status == 0) continue; 

            if (robots[i].x == robots[i].last_x && robots[i].y == robots[i].last_y) {
                robots[i].stuck_count++;
            } else {
                robots[i].stuck_count = 0;
            }
            robots[i].last_x = robots[i].x;
            robots[i].last_y = robots[i].y;

            occupied[robots[i].x][robots[i].y] = false;

            int move_dir = -1;
            bool action_taken = false;
            int tx = -1, ty = -1;

            if (robots[i].has_goods) {
                int best_dist = 1e9;
                int target_x = -1, target_y = -1;
                
                for (auto& b : berths) {
                    int d = abs(robots[i].x - b.first) + abs(robots[i].y - b.second);
                    if (d < best_dist) {
                        best_dist = d;
                        target_x = b.first;
                        target_y = b.second;
                    }
                }

                if (target_x != -1) {
                    tx = target_x; ty = target_y;
                    if (robots[i].x == target_x && robots[i].y == target_y) {
                        cout << "pull " << i << endl;
                        action_taken = true;
                    } else {
                        move_dir = bfs(robots[i].x, robots[i].y, target_x, target_y);
                    }
                }
            } else {
                int target_idx = robot_target_good[i];
                if (target_idx != -1) {
                    tx = goods_list[target_idx].x; ty = goods_list[target_idx].y;
                    if (robots[i].x == goods_list[target_idx].x && robots[i].y == goods_list[target_idx].y) {
                        cout << "get " << i << endl;
                        action_taken = true;
                    } else {
                        move_dir = bfs(robots[i].x, robots[i].y, goods_list[target_idx].x, goods_list[target_idx].y);
                    }
                }
            }

            if (!action_taken) {
                int final_move_dir = -1;

                if (move_dir != -1) {
                    int nx = robots[i].x + dx[move_dir];
                    int ny = robots[i].y + dy[move_dir];
                    if (!occupied[nx][ny]) {
                        final_move_dir = move_dir;
                    }
                }

                if (final_move_dir == -1 && robots[i].stuck_count >5) {
                    vector<int> alt_dirs;
                    for (int d = 0; d < 4; d++) {
                        if (d == move_dir) continue;
                        
                        int nx = robots[i].x + dx[d];
                        int ny = robots[i].y + dy[d];
                        
                        if (nx >= 0 && nx < N && ny >= 0 && ny < N && 
                            grid[nx][ny] != '*' && grid[nx][ny] != '#' && !occupied[nx][ny]) {
                            alt_dirs.push_back(d);
                        }
                    }

                    if (!alt_dirs.empty()) {
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
                    occupied[robots[i].x][robots[i].y] = true;
                }
            } else {
                occupied[robots[i].x][robots[i].y] = true;
            }
        }

        for (int i = 0; i < SHIP_NUM; i++) {
            cout << "go " << i << endl;
        }
        cout << "OK" << endl;
    }
    return 0;
}
