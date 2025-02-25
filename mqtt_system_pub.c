#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mosquitto.h>
#include <unistd.h>

#define MQTT_HOST "broker.hivemq.com"
#define MQTT_PORT 1883

#define MQTT_TOPIC_TEMP "HIEU/TEMP"
#define MQTT_TOPIC_FAN "HIEU/FAN_SPEED"
#define MQTT_TOPIC_RAM "HIEU/RAM_USED"
#define MQTT_TOPIC_RAM_FREE "HIEU/RAM_FREE"
#define MQTT_TOPIC_DISK "HIEU/DISK_USED"
#define MQTT_TOPIC_DISK_FREE "HIEU/DISK_FREE"
#define MQTT_TOPIC_IP "HIEU/IP_ADDRESS"
#define MQTT_TOPIC_CPU "HIEU/CPU_LOAD"
#define MQTT_TOPIC_UPTIME "HIEU/UPTIME"
#define MQTT_TOPIC_OS "HIEU/OS_INFO"

char* exec_cmd(const char *cmd) {
    FILE *fp;
    char buffer[256];
    char *result = (char *)malloc(1024);
    result[0] = '\0';

    fp = popen(cmd, "r");
    if (fp == NULL) return strdup("Error");

    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        strcat(result, buffer);
    }
    pclose(fp);
    return result;
}

int read_value_from_file(const char *filepath) {
    FILE *fp = fopen(filepath, "r");
    if (fp == NULL) return -1;
    int value;
    if (fscanf(fp, "%d", &value) != 1) {
        fclose(fp);
        return -1;
    }
    fclose(fp);
    return value;
}

void publish_data(struct mosquitto *mosq, const char *topic, const char *payload) {
    mosquitto_publish(mosq, NULL, topic, strlen(payload), payload, 0, false);
    printf("Sent [%s]: %s\n", topic, payload);
}

int main() {
    struct mosquitto *mosq;
    mosquitto_lib_init();
    mosq = mosquitto_new(NULL, true, NULL);
    if (!mosq) {
        fprintf(stderr, "Lỗi tạo MQTT client\n");
        return 1;
    }

    if (mosquitto_connect(mosq, MQTT_HOST, MQTT_PORT, 60) != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "Không thể kết nối MQTT Broker\n");
        return 1;
    }

    while (1) {
        char payload[128];

        int temp = read_value_from_file("/sys/class/thermal/thermal_zone0/temp");
        snprintf(payload, sizeof(payload), "%.1f", temp / 1000.0);
        publish_data(mosq, MQTT_TOPIC_TEMP, payload);

        int fan_speed = read_value_from_file("/sys/class/hwmon/hwmon4/fan1_input");
        snprintf(payload, sizeof(payload), "%d", fan_speed);
        publish_data(mosq, MQTT_TOPIC_FAN, payload);

        char *ram_used = exec_cmd("free -m | awk 'NR==2 {print $3}'");
        publish_data(mosq, MQTT_TOPIC_RAM, ram_used);
        free(ram_used);

        char *ram_free = exec_cmd("free -m | awk 'NR==2 {print $4}'");
        publish_data(mosq, MQTT_TOPIC_RAM_FREE, ram_free);
        free(ram_free);

        char *disk_used = exec_cmd("df -h --output=used / | tail -n 1");
        publish_data(mosq, MQTT_TOPIC_DISK, disk_used);
        free(disk_used);

        char *disk_free = exec_cmd("df -h --output=avail / | tail -n 1");
        publish_data(mosq, MQTT_TOPIC_DISK_FREE, disk_free);
        free(disk_free);

        char *ip_info = exec_cmd("hostname -I | awk '{print $1}'");
        publish_data(mosq, MQTT_TOPIC_IP, ip_info);
        free(ip_info);

        char *cpu_load = exec_cmd("top -bn1 | grep 'Cpu(s)' | awk '{print $2}'");
        publish_data(mosq, MQTT_TOPIC_CPU, cpu_load);
        free(cpu_load);

        char *uptime_info = exec_cmd("uptime -p");
        publish_data(mosq, MQTT_TOPIC_UPTIME, uptime_info);
        free(uptime_info);

        char *os_info = exec_cmd("cat /etc/os-release | grep PRETTY_NAME | cut -d '=' -f2");
        publish_data(mosq, MQTT_TOPIC_OS, os_info);
        free(os_info);

        sleep(5);
    }

    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();
    return 0;
}
