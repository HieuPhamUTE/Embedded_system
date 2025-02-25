<?php
// Kết nối MySQL
$servername = "localhost";
$username = "HieuCDT24";
$password = "h";
$database = "system_monitor";

$conn = new mysqli($servername, $username, $password, $database);

// Kiểm tra kết nối
if ($conn->connect_error) {
    die(json_encode(["error" => "Kết nối thất bại: " . $conn->connect_error]));
}

// Truy vấn dữ liệu mới nhất
$sql = "SELECT * FROM system_metrics ORDER BY id DESC LIMIT 30";
$result = $conn->query($sql);

$data = [];
if ($result->num_rows > 0) {
    while ($row = $result->fetch_assoc()) {
        $data[] = $row;
    }
}

// Trả về dữ liệu JSON
header('Content-Type: application/json');
echo json_encode($data);

$conn->close();
?>
