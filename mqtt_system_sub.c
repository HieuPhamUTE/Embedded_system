#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mosquitto.h>
#include <mysql/mysql.h>

#define MQTT_HOST "broker.hivemq.com"
#define MQTT_PORT 1883

// Cấu hình MySQL
#define MYSQL_HOST "localhost"
#define MYSQL_USER "HieuCDT24"
#define MYSQL_PASS "h"
#define MYSQL_DB "system_monitor"

// Biến toàn cục để lưu giá trị nhận được
float temperature = -1;
int fan_speed = -1;
int ram_used = -1, ram_free = -1;
char disk_used[10] = "", disk_free[10] = "";
float cpu_load = -1;
char ip_address[20] = "", uptime[100] = "", os_info[100] = "";

// Hàm lưu dữ liệu vào MySQL
void save_to_mysql() {
    MYSQL *conn = mysql_init(NULL);
    if (!conn) {
        fprintf(stderr, "Lỗi khởi tạo MySQL\n");
        return;
    }

    if (!mysql_real_connect(conn, MYSQL_HOST, MYSQL_USER, MYSQL_PASS, MYSQL_DB, 0, NULL, 0)) {
        fprintf(stderr, "Lỗi kết nối MySQL: %s\n", mysql_error(conn));
        mysql_close(conn);
        return;
    }

    char query[1024];
    snprintf(query, sizeof(query),
        "INSERT INTO system_metrics (temperature, fan_speed, ram_used, ram_free, disk_used, disk_free, cpu_load, ip_address, uptime, os_info) "
        "VALUES (%.1f, %d, %d, %d, '%s', '%s', %.1f, '%s', '%s', '%s')",
        temperature, fan_speed, ram_used, ram_free, disk_used, disk_free, cpu_load, ip_address, uptime, os_info);

    if (mysql_query(conn, query)) {
        fprintf(stderr, "❌ Lỗi INSERT: %s\n", mysql_error(conn));
    } else {
        printf("✅ Dữ liệu đã được lưu vào MySQL!\n");
    }

    mysql_close(conn);
}

// Hàm xử lý khi nhận dữ liệu từ MQTT
void on_message(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *msg) {
    char *topic = msg->topic;
    char message[100];  // Để tránh lỗi tràn bộ nhớ
    snprintf(message, sizeof(message), "%.*s", msg->payloadlen, (char *)msg->payload);

    printf("📩 Nhận MQTT: [%s] %s\n", topic, message);

    // Xác định topic và cập nhật biến
    if (strcmp(topic, "HIEU/TEMP") == 0) {
        temperature = atof(message);
    } else if (strcmp(topic, "HIEU/FAN_SPEED") == 0) {
        fan_speed = atoi(message);
    } else if (strcmp(topic, "HIEU/RAM_USED") == 0) {
        ram_used = atoi(message);
    } else if (strcmp(topic, "HIEU/RAM_FREE") == 0) {
        ram_free = atoi(message);
    } else if (strcmp(topic, "HIEU/DISK_USED") == 0) {
        strncpy(disk_used, message, sizeof(disk_used) - 1);
        disk_used[sizeof(disk_used) - 1] = '\0';  // Đảm bảo kết thúc chuỗi
    } else if (strcmp(topic, "HIEU/DISK_FREE") == 0) {
        strncpy(disk_free, message, sizeof(disk_free) - 1);
        disk_free[sizeof(disk_free) - 1] = '\0';
    } else if (strcmp(topic, "HIEU/CPU_LOAD") == 0) {
        cpu_load = atof(message);
    } else if (strcmp(topic, "HIEU/IP_ADDRESS") == 0) {
        strncpy(ip_address, message, sizeof(ip_address) - 1);
        ip_address[sizeof(ip_address) - 1] = '\0';
    } else if (strcmp(topic, "HIEU/UPTIME") == 0) {
        strncpy(uptime, message, sizeof(uptime) - 1);
        uptime[sizeof(uptime) - 1] = '\0';
    } else if (strcmp(topic, "HIEU/OS_INFO") == 0) {
        strncpy(os_info, message, sizeof(os_info) - 1);
        os_info[sizeof(os_info) - 1] = '\0';
    }

    // Lưu vào MySQL sau khi nhận dữ liệu
    save_to_mysql();
}

int main() {
    struct mosquitto *mosq;

    // Khởi tạo thư viện MQTT
    mosquitto_lib_init();
    mosq = mosquitto_new(NULL, true, NULL);
    if (!mosq) {
        fprintf(stderr, "❌ Lỗi tạo MQTT client\n");
        return 1;
    }

    // Đặt callback xử lý tin nhắn
    mosquitto_message_callback_set(mosq, on_message);

    // Kết nối đến MQTT Broker
    if (mosquitto_connect(mosq, MQTT_HOST, MQTT_PORT, 60) != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "❌ Không thể kết nối MQTT Broker\n");
        return 1;
    }

    // Đăng ký các topic
    char *topics[] = {
        "HIEU/TEMP", "HIEU/FAN_SPEED", "HIEU/RAM_USED", "HIEU/RAM_FREE",
        "HIEU/DISK_USED", "HIEU/DISK_FREE", "HIEU/CPU_LOAD",
        "HIEU/IP_ADDRESS", "HIEU/UPTIME", "HIEU/OS_INFO"
    };

    int num_topics = sizeof(topics) / sizeof(topics[0]);
    for (int i = 0; i < num_topics; i++) {
        mosquitto_subscribe(mosq, NULL, topics[i], 0);
    }

    printf("🚀 Đang lắng nghe dữ liệu từ MQTT...\n");

    // Lắng nghe MQTT liên tục
    mosquitto_loop_forever(mosq, -1, 1);

    // Dọn dẹp tài nguyên
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();
    return 0;
}
