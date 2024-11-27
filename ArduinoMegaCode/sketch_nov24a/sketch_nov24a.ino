#define RXp2 16
#define TXp2 17
#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h>
MCUFRIEND_kbv tft;

// Screen colors
#define RED 0xF800
#define ORANGE 0xFDA0  // RGB(255, 165, 0)
#define YELLOW 0xFFE0
#define GREEN 0x07E0
#define BLUE 0x001F
#define MAGENTA 0xF81F
#define CYAN 0x07FF
#define WHITE 0xFFFF
#define BLACK 0x0000
#define PURPLE 0x780F  // RGB(128, 0, 128)
#define BROWN 0xA145  // RGB(165, 42, 42)
#define PINK 0xFC9F  // RGB(255, 192, 203)
#define LIGHT_BLUE 0xAEDC  // RGB(173, 216, 230)
#define DARK_GREEN 0x0320  // RGB(0, 100, 0)

// 定义按钮引脚
const int buttonPin1 = 22;  // 按钮1连接到数字引脚 D22
const int buttonPin2 = 24;  // 按钮2连接到数字引脚 D24

// 变量存储按钮状态
bool button1State = false;
bool button2State = false;
int button1Times = 0;
int button2Times = 0;
unsigned long previousPressedbt1 = 0;
unsigned long previousPressedbt2 = 0;
bool isUARTActive = false; // 标志位，指示是否正在进行 UART 通信

// Time-based chart switch
unsigned long previousMillis = 0;
const long interval = 10000;  // 10000 ms = 10 seconds (change this value to 10 seconds)
int chartIndex = 0;  // Current chart index


// 定义一个结构体来存储WiFi信息
struct WiFiInfo {
  String ssid;
  int signalStrength;
  int encryptionType;
  String bssid;
  int channel;
  int frequency;
  unsigned long lastUpdated;  // 记录最后更新时间
};

// 最大支持的 Wi-Fi 网络数目
#define MAX_NETWORKS 100

WiFiInfo receivedNetworks[MAX_NETWORKS];  // 用来存储 Wi-Fi 信息的数组
int receivedNetworksCount = 0;  // 当前存储的 Wi-Fi 信息数量

// 用来存储每个SSID的最近三次信号强度
struct SSIDSignalStrength {
  String ssid;
  int signalStrengths[3];  // 环形缓冲区，保存最近三次信号强度
  int index;               // 当前存储位置
  unsigned long lastUpdated;  // 记录最后更新时间
  int avgSignalStrength; // 信号强度平均值
};

SSIDSignalStrength ssidSignalStrengths[MAX_NETWORKS];  // 存储 SSID 的信号强度数据
int ssidSignalStrengthsCount = 0;  // 当前存储的 SSID 数量

unsigned long lastCalculationTime = 0;
const unsigned long calculationInterval = 3000;  // 每3秒计算一次平均信号强度
const unsigned long timeoutInterval = 10000;    // 10秒超时删除


// 按信号强度降序排序
void sortNetworks() {
  for (int i = 0; i < receivedNetworksCount - 1; i++) {
    for (int j = i + 1; j < receivedNetworksCount; j++) {
      if (receivedNetworks[i].signalStrength < receivedNetworks[j].signalStrength) {
        WiFiInfo temp = receivedNetworks[i];
        receivedNetworks[i] = receivedNetworks[j];
        receivedNetworks[j] = temp;
      }
    }
  }
}

// 按平均信号强度排序
void sortAvgGain() {
  for (int i = 0; i < ssidSignalStrengthsCount - 1; i++) {
    for (int j = i + 1; j < ssidSignalStrengthsCount; j++) {
      if (ssidSignalStrengths[i].avgSignalStrength < ssidSignalStrengths[j].avgSignalStrength) {
        SSIDSignalStrength temp = ssidSignalStrengths[i];
        ssidSignalStrengths[i] = ssidSignalStrengths[j];
        ssidSignalStrengths[j] = temp;
      }
    }
  }
}

// 查找并更新 SSID 的信号强度数据
void updateSSIDSignalStrength(String ssid, int signalStrength) {
  bool found = false;
  for (int i = 0; i < ssidSignalStrengthsCount; i++) {
    if (ssidSignalStrengths[i].ssid == ssid) {
      // 找到对应的 SSID，更新信号强度数据（环形缓冲区）
      ssidSignalStrengths[i].signalStrengths[ssidSignalStrengths[i].index] = signalStrength;
      ssidSignalStrengths[i].index = (ssidSignalStrengths[i].index + 1) % 3;  // 更新存储位置，形成循环

      ssidSignalStrengths[i].lastUpdated = millis();  // 更新最后更新时间
      ssidSignalStrengths[i].avgSignalStrength = calculateAverageSignalStrength(ssid);
      found = true;
      break;
    }
  }
  
  if (!found && ssidSignalStrengthsCount < MAX_NETWORKS) {
    // 如果未找到该 SSID，新增记录
    ssidSignalStrengths[ssidSignalStrengthsCount].ssid = ssid;
    ssidSignalStrengths[ssidSignalStrengthsCount].signalStrengths[0] = signalStrength;
    ssidSignalStrengths[ssidSignalStrengthsCount].signalStrengths[1] = 0;
    ssidSignalStrengths[ssidSignalStrengthsCount].signalStrengths[2] = 0;
    ssidSignalStrengths[ssidSignalStrengthsCount].index = 1;
    ssidSignalStrengths[ssidSignalStrengthsCount].avgSignalStrength = signalStrength;
    ssidSignalStrengths[ssidSignalStrengthsCount].lastUpdated = millis();  // 记录初次更新时间
    ssidSignalStrengthsCount++;
  }
}

// 计算某个SSID最近三次信号强度的平均值
int calculateAverageSignalStrength(String ssid) {
  for (int i = 0; i < ssidSignalStrengthsCount; i++) {
    if (ssidSignalStrengths[i].ssid == ssid) {
      int sum = 0;
      int count = 0;
      // 计算信号强度的总和，排除0的无效值
      for (int j = 0; j < 3; j++) {
        if (ssidSignalStrengths[i].signalStrengths[j] != 0) {
          sum += ssidSignalStrengths[i].signalStrengths[j];
          count++;
        }
      }
      // 计算平均值
      if (count > 0) {
        return sum / count;
      }
    }
  }
  return 0;  // 如果没有找到，返回0
}

// 删除超时的SSID记录
void removeExpiredSSIDs() {
  unsigned long currentTime = millis();
  
  for (int i = 0; i < ssidSignalStrengthsCount; i++) {
    if (currentTime - ssidSignalStrengths[i].lastUpdated >= timeoutInterval) {
      // 删除该SSID的信息，向前移动数组中的记录
      for (int j = i; j < ssidSignalStrengthsCount - 1; j++) {
        ssidSignalStrengths[j] = ssidSignalStrengths[j + 1];  // 向前移动记录
      }
      ssidSignalStrengthsCount--;  // 减少SSID数量
      i--;  // 调整索引，检查下一条记录
    }
  }
}

String formatSSID(String ssid) {
  if (ssid.length() <= 8) {
    return ssid;  // 如果SSID长度小于或等于8个字母，直接返回
  } else {
    // 超过8个字母，显示前四个字母和后三个字母，中间用省略号代替
    return ssid.substring(0, 4) + "..." + ssid.substring(ssid.length() - 3);
  }
}

void monitorButtons() {
  // 非阻塞检查按钮状态
  bool currentButton1State = digitalRead(buttonPin1) == LOW; // 按钮按下时为 LOW
  bool currentButton2State = digitalRead(buttonPin2) == LOW;

  // 检查按钮 1 状态变化
  if (currentButton1State && !button1State) {
    button1State = true;
    Serial.println("Button 1 Pressed!");
    if(button1Times == 0)
    {
      drawChart1();
      button1Times++;
    }else if(button1Times == 1){
      drawChart2();
      button1Times++;
    }else if(button1Times == 2){
      drawChart3();
      button1Times++;
    }else{
      indexpage();
      button1Times = 0;
    }
    previousMillis = millis();
    Serial.print("Pressed Times: ");
    Serial.println(button1Times);
    // 按钮 1 被按下时的处理逻辑
  } else if (!currentButton1State && button1State) {
    button1State = false;
  }

  // 检查按钮 2 状态变化
  if (currentButton2State && !button2State) {
    button2State = true;
    Serial.println("Button 2 Pressed!");
    if(button1Times == 0)
    {  
      indexpage();
    }else if(button1Times == 1){
      drawChart1();
    }else if(button1Times == 2){
      drawChart2();
    }else{
      drawChart3();
    }
    previousMillis = millis();

    // 按钮 2 被按下时的处理逻辑
  } else if (!currentButton2State && button2State) {
    button2State = false;
  }
}

void drawDashedLine(int x0, int y0, int x1, int y1, uint16_t color, int dashLength) {
  // Calculate the total distance between the points
  float dx = x1 - x0;
  float dy = y1 - y0;
  float distance = sqrt(dx * dx + dy * dy);

  // Calculate unit direction vector
  float unitX = dx / distance;
  float unitY = dy / distance;

  // Draw the dashed line
  int numDashes = distance / (2 * dashLength);  // Calculate the number of dashes
  for (int i = 0; i < numDashes; i++) {
    int startX = x0 + unitX * (i * 2 * dashLength);
    int startY = y0 + unitY * (i * 2 * dashLength);
    int endX = x0 + unitX * ((i * 2 + 1) * dashLength);
    int endY = y0 + unitY * ((i * 2 + 1) * dashLength);

    // Draw a segment of the dashed line
    tft.drawLine(startX, startY, endX, endY, color);
  }
}

void drawMarker(int x, int y, uint16_t color) {
  // 绘制一个 5x5 像素的小方块
  for (int dx = -2; dx <= 2; dx++) {
    for (int dy = -2; dy <= 2; dy++) {
      tft.drawPixel(x + dx, y + dy, color);
    }
  }
}

void indexpage(){
  tft.fillScreen(BLACK);
  tft.setTextColor(WHITE);
  tft.setTextSize(3);
  tft.setCursor(10,20);
  tft.print("Environmental");
  tft.setCursor(10,60);
  tft.print("Signal");
  tft.setCursor(10,100);
  tft.print("Analyzer");
  tft.setCursor(10,140);
  tft.setTextColor(MAGENTA);
  tft.setTextSize(5);
  tft.print("CASA0016");
  tft.setTextSize(2);
  tft.setCursor(10,190);
  tft.print("Making, Designing & Building");
  tft.setCursor(10,220);
  tft.print("Connected Sensor Systems");
  tft.setCursor(10,270);
  tft.setTextColor(YELLOW);
  tft.setTextSize(3);
  tft.print("Designed by Sirui Luo");
}

// Draw the first chart -- 前六SSID增益折线图 -- 横竖单位都用3
// 绘制前六个Wi-Fi信号强度折线图
void drawChart1() {
  tft.fillScreen(BLACK);
  tft.setTextColor(WHITE);
  tft.setTextSize(2);
  tft.setCursor(50, 10);  // 调整位置以适应横屏
  tft.print("Chart 1: Line Chart in Ch0~Ch11");
  tft.setTextSize(1);
  tft.setCursor(10, 10);  // 调整位置以适应横屏
  tft.print("Gain");
  tft.setCursor(10, 18);  // 调整位置以适应横屏
  tft.print("/dB");
  tft.setCursor(465, 305);  // 调整位置以适应横屏
  tft.print("Ch");

  // 按信号强度降序排序
  sortNetworks();

  // 选择折线的颜色
  uint16_t chartColors[] = {RED, ORANGE, YELLOW, GREEN, BLUE, MAGENTA};
  tft.drawLine(5,315,475,315,WHITE);//x轴
  tft.drawLine(5,5,5,315,WHITE);//y轴
  // 绘制折线图
  //int count = min(6, receivedNetworksCount);  // 如果Wi-Fi网络少于6个，则绘制实际的网络数
  int minSignalStrength = 0;
  int maxSignalStrength = -100;

  // 遍历找到最低和最高信号强度
  for (int i = 0; i < 6; i++) {
    WiFiInfo network = receivedNetworks[i];
    if (network.signalStrength < minSignalStrength) {
      minSignalStrength = network.signalStrength;
    }
    if (network.signalStrength > maxSignalStrength) {
      maxSignalStrength = network.signalStrength;
    }
  }

  // 使用找到的最小和最大信号强度进行绘图
  for (int i = 0; i < 6; i++) {
    WiFiInfo network = receivedNetworks[i];

    // 横坐标为信道，纵坐标为增益（信号强度）
    int x = map(network.channel, -1, 12, 5, 475);
    int y = map(network.signalStrength, minSignalStrength - 3, maxSignalStrength + 3, 310, 55);  // 用找到的最小和最大信号强度映射为Y坐标

    // 绘制折线，颜色依次循环
    if (i >= 0) {
      int prevX = x - 10;
      int prevY = y;
      int aftX = x + 10;
      int aftY = y;
      tft.drawLine(prevX, prevY, prevX - 7, 315, chartColors[i]); // 左坡度
      tft.drawLine(aftX, aftY, aftX + 7, 315, chartColors[i]);     // 右坡度
      tft.drawLine(prevX, prevY, aftX, aftY, chartColors[i]);      // 绘制两点之间的折线
      drawDashedLine(5, y, prevX, y, chartColors[i], 3);           // 左侧虚线
      drawDashedLine(x, y, x, 315, chartColors[i], 3);           // 中间虚线
      //tft.setTextColor(chartColors[i]);
      tft.setTextSize(2);
      tft.setCursor(x-3, 300);
      //tft.print("Ch:");
      tft.setTextColor(WHITE);
      tft.print(network.channel);

      tft.setTextSize(1);
      tft.setCursor(10, y);
      //tft.print("Ch:");
      tft.setTextColor(WHITE);
      tft.print(network.signalStrength);
      //tft.print(" S:");
      //tft.print(network.signalStrength);
      if(i >= 0 & i < 3){
      String formattedSSID = formatSSID(network.ssid);
      tft.setTextColor(chartColors[i]);
      tft.setTextSize(1);
      drawMarker(40 + 156*i, 37, chartColors[i]);  // 使用对应的颜色绘制Marker
      tft.setCursor(47 + 156*i, 35);  // 设置光标位置
      tft.print("(");
      tft.print(network.channel);
      tft.print(")");
      tft.setCursor(75 + 156*i, 35);  // 设置光标位置
      tft.print(formattedSSID);  // 显示格式化后的SSID
      }
      if(i >= 3 & i < 6){
      String formattedSSID = formatSSID(network.ssid);
      tft.setTextColor(chartColors[i]);
      tft.setTextSize(1);
      drawMarker(40 + 156*(i-3), 49, chartColors[i]);  // 使用对应的颜色绘制Marker
      tft.setCursor(47 + 156*(i-3), 47);  // 设置光标位置
      tft.print("(");
      tft.print(network.channel);
      tft.print(")");
      tft.setCursor(75 + 156*(i-3), 47);  // 设置光标位置
      tft.print(formattedSSID);  // 显示格式化后的SSID
      }
    }
  }
}


// Draw the second chart
void drawChart2() {
  sortAvgGain();
  tft.fillScreen(BLACK);
  tft.setTextColor(WHITE);
  tft.setTextSize(2);
  tft.setCursor(5, 10);  // 调整位置以适应横屏
  tft.print("Chart 2: Best 6 SSID in Avg Signal Gain");

  uint16_t chartColors[] = {RED, ORANGE, YELLOW, GREEN, BLUE, MAGENTA};
  int minAvgStrength = 0;
  int maxAvgStrength = -100;
  int count = min(6, ssidSignalStrengthsCount);

  for (int i = 0; i < count; i++) {
    SSIDSignalStrength avgGain = ssidSignalStrengths[i];
    if (avgGain.avgSignalStrength < minAvgStrength) {
      minAvgStrength = avgGain.avgSignalStrength;
    }
    if (avgGain.avgSignalStrength > maxAvgStrength) {
      maxAvgStrength = avgGain.avgSignalStrength;
    }
  }

  Serial.println(minAvgStrength);
  Serial.println(maxAvgStrength);

  for (int i = 0; i < count; i++){
    SSIDSignalStrength avgGain = ssidSignalStrengths[i];

    // 横坐标为信道，纵坐标为增益（信号强度）
    int x = map(i, 0, 6, 15, 475);
    int y = map(avgGain.avgSignalStrength, minAvgStrength - 2, maxAvgStrength + 2, 260, 55) - 260;  // 用找到的最小和最大信号强度映射为Y坐标
    int textX = x + 10;
    int textY = 280;

    if (i >= 0) {
      tft.fillRect(x, 270, 50, y, chartColors[i]);
      tft.setCursor(textX, textY);

      //tft.print("Ch:");
      tft.setTextColor(chartColors[i]);
      tft.setTextSize(2);
      tft.print(avgGain.avgSignalStrength);

      if(i >= 0 & i < 3){
      String formattedSSID = formatSSID(avgGain.ssid);
      tft.setTextColor(chartColors[i]);
      tft.setTextSize(1);
      drawMarker(40 + 156*i, 37, chartColors[i]);  // 使用对应的颜色绘制Marker
      tft.setCursor(60 + 156*i, 35);  // 设置光标位置
      tft.print(formattedSSID);  // 显示格式化后的SSID
      }
      if(i >= 3 & i < 6){
      String formattedSSID = formatSSID(avgGain.ssid);
      tft.setTextColor(chartColors[i]);
      tft.setTextSize(1);
      drawMarker(40 + 156*(i-3), 49, chartColors[i]);  // 使用对应的颜色绘制Marker
      tft.setCursor(60 + 156*(i-3), 47);  // 设置光标位置
      tft.print(formattedSSID);  // 显示格式化后的SSID
      }
    }
  }
  


  // Draw a simple bar chart
  //tft.fillRect(10, 300, 50, -100, RED);
  //tft.fillRect(70, 300, 50, -150, GREEN);
  //tft.fillRect(130, 300, 50, -50, BLUE);
}

// Draw the third chart
void drawChart3() {
  tft.fillScreen(BLACK);
  tft.setTextColor(WHITE);
  tft.setTextSize(2);
  tft.setCursor(40, 10);  // 调整位置以适应横屏
  tft.print("Chart 3: Encryption and Frequency");

  sortNetworks();

  // 选择折线的颜色
  uint16_t chartColors[] = {RED, BROWN, ORANGE, YELLOW, DARK_GREEN, GREEN, CYAN, LIGHT_BLUE, BLUE, PINK, MAGENTA, PURPLE};
  int containitems = 0;
  int encryption0Count = 0;

  tft.setCursor(10, 60);  // 调整位置以适应横屏
  tft.print("1.Encryption Type");

  for (int i = 0; i < MAX_NETWORKS; i++) {
    WiFiInfo network = receivedNetworks[i];
    if(network.signalStrength != 0)
    {
      containitems++;
    }
  }

  for (int i = 0; i < containitems; i++) {
    WiFiInfo network = receivedNetworks[i];
    if(network.encryptionType == 0)
    {
      encryption0Count++;
    }
  }

  float Ratio = 0;
  if (containitems != 0) {
    Ratio = (float)encryption0Count / containitems;

    // 保留两位小数
    Ratio = round(Ratio * 100) / 100.0;
    Ratio = Ratio * 100;
  }

  Serial.println(encryption0Count);
  Serial.println(containitems);
  Serial.println(Ratio);

  tft.setCursor(230, 60);
  tft.setTextColor(RED);
  tft.print("No Encryption:");
  tft.print(Ratio);  // 指定保留两位小数
  tft.print("%");

  int x = map(encryption0Count, 0, containitems, 15, 465);
  int y = 90; 
  tft.fillRect(15, y, x-15, 20, RED);
  tft.fillRect(x, y-1, 465-x, 20, BLUE);

  tft.setTextColor(WHITE);
  tft.setCursor(10, 190);  // 调整位置以适应横屏
  tft.print("2.Frequency Distribution");

  x = map(encryption0Count, 0, containitems, 15, 465);
  y = 220; 
  tft.fillRect(15, y, 30, 20, RED);
  tft.fillRect(45, y, 30, 20, ORANGE);
  tft.fillRect(75, y, 60, 20, YELLOW);
  tft.fillRect(135, y, 30, 20, GREEN);
  tft.fillRect(165, y, 60, 20, CYAN);
  tft.fillRect(225, y, 70, 20, BLUE);
  tft.fillRect(295, y, 170, 20, PURPLE);
}

void setup() {
  Serial.begin(9600);
  Serial1.begin(115200);
  uint16_t ID = tft.readID();
  tft.begin(ID);
  tft.setRotation(3);  // 横屏模式
  tft.fillScreen(BLACK);

  // Instructions for user
  tft.setTextColor(WHITE);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.print("Project Loading...");
  pinMode(buttonPin1, INPUT);
  pinMode(buttonPin2, INPUT);
  Serial.println("Arduino Mega Ready");
  delay(5000);
}


void loop() {
  // Time-based chart switch
  unsigned long currentMillis = millis();

  // 检查是否从 ESP32 接收到数据
  if (Serial1.available()) {
    isUARTActive = true; // 设置标志，表示 UART 通信开始
    String data = Serial1.readStringUntil('\n');  // 读取一行数据
    if (data != "") {
      // 分割数据
      int idx1 = data.indexOf('|');
      int idx2 = data.indexOf('|', idx1 + 1);
      int idx3 = data.indexOf('|', idx2 + 1);
      int idx4 = data.indexOf('|', idx3 + 1);
      int idx5 = data.indexOf('|', idx4 + 1);

      if (idx5 != -1) {
        // 解析Wi-Fi信息
        WiFiInfo currentNetwork = {
          data.substring(0, idx1),
          data.substring(idx1 + 1, idx2).toInt(),
          data.substring(idx2 + 1, idx3).toInt(),
          data.substring(idx3 + 1, idx4),
          data.substring(idx4 + 1, idx5).toInt(),
          data.substring(idx5 + 1).toInt(),
          millis()  // 记录数据的接收时间
        };

        // 将Wi-Fi信息添加到数组
        if (receivedNetworksCount < MAX_NETWORKS) {
          receivedNetworks[receivedNetworksCount] = currentNetwork;
          receivedNetworksCount++;
        }

        // 更新SSID的信号强度数据
        updateSSIDSignalStrength(currentNetwork.ssid, currentNetwork.signalStrength);
      }
    }
    isUARTActive = false; // 通信结束，恢复标志
  }else{
    // 当没有 UART 通信时才监控按钮状态
    if (!isUARTActive) {
    monitorButtons();
    }
  }

  // 每3秒计算一次平均信号强度并显示
  if (millis() - lastCalculationTime >= calculationInterval) {
    lastCalculationTime = millis();

    // 每次收到新数据后，进行排序和显示
    if (receivedNetworksCount > 0) {
      // 按信号强度排序
      sortNetworks();
      // 排序平均信号强度
      sortAvgGain();

      // 显示信号最强的前6个Wi-Fi网络
      int count = min(6, receivedNetworksCount);
      for (int i = 0; i < count; ++i) {
        WiFiInfo network = receivedNetworks[i];
        String wifiInfo = "SSID: " + network.ssid
                          + " | Signal: " + String(network.signalStrength)
                          + " | Encryption: " + String(network.encryptionType)
                          + " | BSSID: " + network.bssid
                          + " | Channel: " + String(network.channel)
                          + " | Frequency: " + String(network.frequency) + " MHz";

        Serial.println(wifiInfo);  // 输出到Arduino Mega串口监视器
      }

      // 输出前六个SSID的最近三次信号强度的平均值
      count = min(6, ssidSignalStrengthsCount);
      for (int i = 0; i < count; ++i){
        SSIDSignalStrength avgGain = ssidSignalStrengths[i];
        String summaryInfo = "SSID: " + String(avgGain.ssid) + " | Average Signal Strength: " + String(avgGain.avgSignalStrength);
        Serial.println(summaryInfo);  // 输出到Arduino Mega串口监视器
      }

      // 清空数据，为下一轮扫描做准备
      receivedNetworksCount = 0;
    }
  }

  // 每秒删除超时的SSID记录
  removeExpiredSSIDs();

  //自动刷新
  currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    if(button1Times == 0)
    {  
      indexpage();
    }else if(button1Times == 1){
      drawChart1();
    }else if(button1Times == 2){
      drawChart2();
    }else{
      drawChart3();
    }
  }
}
