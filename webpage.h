const char PAGE_HTML[] PROGMEM = R"====(
<!DOCTYPE html>
<html lang="vi">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Chọn Vị Trí</title>
    <style>
        /* Nền trang với hiệu ứng gradient động */
        body {
            font-family: Arial, sans-serif;
            background: linear-gradient(135deg, #00aaff, #ff77aa, #ffaa00);
            background-size: 300% 300%;
            animation: gradientAnimation 10s ease infinite;
            display: flex;
            justify-content: center;
            align-items: center;
            height: 100vh;
            color: #fff;
            overflow: hidden;
        }

        @keyframes gradientAnimation {
            0% { background-position: 0% 50%; }
            50% { background-position: 100% 50%; }
            100% { background-position: 0% 50%; }
        }

        /* Khung chứa nội dung */
        .container {
            text-align: center;
            background: rgba(255, 255, 255, 0.2);
            backdrop-filter: blur(12px);
            padding: 30px;
            border-radius: 20px;
            box-shadow: 0 8px 32px rgba(31, 38, 135, 0.37);
            width: 90%;
            max-width: 400px;
            animation: scaleIn 1s ease-in-out;
        }

        h1 {
            color: #ffffff;
            font-size: 28px;
            margin-bottom: 20px;
            animation: fadeInDown 1s ease-out;
        }
        
        select, input[type="submit"] {
            padding: 12px;
            margin-top: 15px;
            border-radius: 8px;
            border: none;
            font-size: 16px;
            width: 100%;
            transition: transform 0.3s, background-color 0.3s;
            animation: slideInUp 0.6s ease-in-out;
        }

        select {
            background-color: rgba(255, 255, 255, 0.8);
            color: #333;
            font-weight: bold;
        }

        input[type="submit"] {
            background-color: rgba(30, 144, 255, 0.9);
            color: #ffffff;
            font-weight: bold;
            cursor: pointer;
        }

        input[type="submit"]:hover {
            background-color: rgba(0, 123, 255, 1);
            transform: scale(1.05);
        }

        /* Phần thông tin hiển thị */
        .info {
            margin-top: 20px;
            font-size: 16px;
            color: #eee;
            animation: fadeIn 1.2s ease-in-out;
        }

        .highlight {
            font-weight: bold;
            color: #00c6ff;
        }

        /* Màu động cho nhiệt độ */
        .temp-blue {
            color: #1e90ff; /* Xanh nước cho 20-24 */
        }
        .temp-green {
            color: #32cd32; /* Xanh lá cho 24-27 */
        }
        .temp-yellow {
            color: #ffcc00; /* Vàng đỏ cho 27-30 */
        }
        .temp-red {
            color: #ff4500; /* Đỏ cho 30-35 */
        }

        /* Modal khi cập nhật thành công */
        .modal {
            display: none;
            position: fixed;
            z-index: 1;
            left: 0;
            top: 0;
            width: 100%;
            height: 100%;
            background-color: rgba(0, 0, 0, 0.6);
            justify-content: center;
            align-items: center;
        }

        .modal-content {
            background: rgba(255, 255, 255, 0.85);
            padding: 20px;
            border-radius: 10px;
            width: 80%;
            max-width: 300px;
            text-align: center;
            animation: scaleIn 0.5s ease-in-out;
        }

        .modal-content h2 {
            color: #4CAF50;
            margin-bottom: 20px;
        }

        .modal-content button {
            padding: 10px 20px;
            background-color: #4CAF50;
            color: white;
            border: none;
            border-radius: 5px;
            font-size: 16px;
            cursor: pointer;
        }

        /* Keyframes cho hiệu ứng động */
        @keyframes fadeInDown {
            from { opacity: 0; transform: translateY(-20px); }
            to { opacity: 1; transform: translateY(0); }
        }

        @keyframes slideInUp {
            from { opacity: 0; transform: translateY(20px); }
            to { opacity: 1; transform: translateY(0); }
        }

        @keyframes scaleIn {
            from { opacity: 0; transform: scale(0.9); }
            to { opacity: 1; transform: scale(1); }
        }
    </style>
    <script>
        let initialTime = new Date("{{current_time}}");

        function updateTime() {
            initialTime.setSeconds(initialTime.getSeconds() + 1);
            document.getElementById("time").innerText = initialTime.toLocaleString("vi-VN");
        }

        setInterval(updateTime, 1000); 

        function showSuccessMessage() {
            document.getElementById("successModal").style.display = "flex";
            setTimeout(() => { window.location.href = "/"; }, 2000);
        }

        function closeModal() {
            document.getElementById("successModal").style.display = "none";
        }

        // Cập nhật màu của nhiệt độ dựa trên giá trị
        function updateTemperatureColor() {
            const tempElement = document.getElementById("temperature");
            const temperature = parseFloat(tempElement.innerText);

            if (temperature >= 20 && temperature < 24) {
                tempElement.className = "highlight temp-blue";
            } else if (temperature >= 24 && temperature < 27) {
                tempElement.className = "highlight temp-green";
            } else if (temperature >= 27 && temperature < 30) {
                tempElement.className = "highlight temp-yellow";
            } else if (temperature >= 30) {
                tempElement.className = "highlight temp-red";
            }
        }

        window.onload = updateTemperatureColor;
    </script>
</head>
<body>
    <div class="container">
        <h1>Chọn Vị Trí</h1>
        <form action="/setLocation" method="POST" onsubmit="showSuccessMessage()">
            <select name="location">
                {{location_options}}
            </select>
            <input type="submit" value="Cập nhật">
        </form>
        <div class="info">
            <p><strong>Vị trí đang chọn hiện tại:</strong> <span class="highlight">{{current_location}}</span></p>
            <p><strong>Thời gian hiện tại:</strong> <span id="time" class="highlight">{{current_time}}</span></p>
            <p><strong>Nhiệt độ hiện tại:</strong> <span id="temperature" class="highlight">{{temperature}}</span>°C</p>
        </div>
    </div>

    <div id="successModal" class="modal">
        <div class="modal-content">
            <h2>Đã cập nhật thành công</h2>
            <button onclick="closeModal()">OK</button>
        </div>
    </div>
</body>
</html>
)====";
