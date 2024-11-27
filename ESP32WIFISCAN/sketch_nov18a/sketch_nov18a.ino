#include <WiFi.h>

#define RXp2 16
#define TXp2 17

// 定义一个结构体来存储WiFi信息
struct WiFiInfo {
  String ssid;
  int signalStrength;
  int encryptionType;
  String bssid;
  int channel;
  int frequency;
};

// 获取信道对应的频率
int getFrequency(int channel) {
  if (channel >= 1 && channel <= 11) {
    return 2407 + channel * 5;  // 计算2.4GHz信道的频率
  } else if (channel >= 36 && channel <= 165) {
    return 5000 + channel * 5;  // 计算5GHz信道的频率
  }
  return -1;  // 未知信道，返回无效值
}

unsigned long lastScanTime = 0;
const unsigned long scanInterval = 3000;  // 每3秒刷新一次

void setup() {
  Serial.begin(9600);
  Serial2.begin(115200, SERIAL_8N1, RXp2, TXp2);

  WiFi.mode(WIFI_STA);   // 设置ESP32为客户端模式
  WiFi.disconnect();     // 断开所有网络连接
  delay(100);
}

void loop() {
  if (millis() - lastScanTime >= scanInterval) {
    lastScanTime = millis();

    int n = WiFi.scanNetworks();
    Serial.println("Scan done");

    if (n == 0) {
      Serial.println("No networks found");
      Serial2.println("No networks found");
    } else {
      // 将每个网络的信息发送给 Arduino Mega
      for (int i = 0; i < n; ++i) {
        int channel = WiFi.channel(i);
        int frequency = getFrequency(channel);

        // 格式化Wi-Fi信息
        String wifiInfo = WiFi.SSID(i) + "|" + String(WiFi.RSSI(i)) + "|" +
                          String(WiFi.encryptionType(i)) + "|" + WiFi.BSSIDstr(i) + "|" +
                          String(channel) + "|" + String(frequency);

        // 在ESP32串口监视器中输出信息，确保它正确发送
        Serial.println("Sending to Arduino Mega: ");
        Serial.println(wifiInfo);

        // 发送Wi-Fi信息给Arduino Mega
        Serial2.println(wifiInfo);
      }
    }
  }
}
