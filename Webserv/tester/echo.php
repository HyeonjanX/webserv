<?php
// HTTP 응답 헤더 설정



header('Content-Type: text/plain');

echo "Content-Type: text/plain\r\n";

if ($_SERVER['REQUEST_METHOD'] === 'GET') {

    echo "Status: 200 OK\r\n";
    echo "\r\n";
    echo "No content (GET 요청)";

} elseif ($_SERVER['REQUEST_METHOD'] === 'POST') {

    $handle = fopen('php://stdin', 'r');
    echo "Status: 200 OK\r\n";
    echo "\r\n";

    while (!feof($handle)) {
        $buffer = fread($handle, 1024);
        if ($buffer !== false) {
            echo $buffer;
        }
    }

    fclose($handle);

} else {

    echo "Status: 500 Unsupported HTTP method\r\n";
    echo "\r\n";
    echo "지원하지 않는 메서드 입니다.";

}
?>