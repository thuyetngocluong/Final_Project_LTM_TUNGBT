# Final_Project_LTM_TUNGBT
Final project of Network Programming in HUST </br>
A Caro Game Online using winsock

# Members
THUYET LUONG NGOC <br/>  
TUAN NGUYEN MINH <br/>  
ANH NGUYEN HONG <br/>  
# Protocol
### 1. LOGIN
|Command|OpCode|Use By|
|---|---|---|
|REQ_LOGIN|10|Client|

|Command|ResponseCode|Use By|
|---|---|---|
|RES_LOGIN_SUCCESSFUL|100|Server|
|RES_LOGIN_FAIL|101|Server|
|RES_LOGIN_FAIL_LOCKED|102|Server|
|RES_LOGIN_FAIL_WRONG_PASS|103|Server|
|RES_LOGIN_FAIL_NOT_EXIST|104|Server|
|RES_LOGIN_FAIL_ANOTHER_DEVICE|105|Server|
|RES_LOGIN_FAIL_ALREADY_LOGIN|106|Server|

### 2. LOGOUT
|Command|OpCode|Use By|
|---|---|---|
|REQ_LOGOUT|11|Client|

|Command|ResponseCode|Use By|
|---|---|---|
|RES_LOGOUT_SUCCESSFUL|110|Server|
|RES_LOGOUT_FAIL|111|Server|

 ### 3. REGISTER
|Command|OpCode|Use By|
|---|---|---|
|REQ_REGISTER|12|Client|

|Command|ResponseCode|Use By|
|---|---|---|
|RES_REGISTER_SUCCESSFUL|120|Server|
|RES_REGISTER_FAIL|121|Server|

### 4. GET LIST FRIEND
|Command|OpCode|Use By|
|---|---|---|
|REQ_GET_LIST|20|Client|

|Command|ResponseCode|Use By|
|---|---|---|
|RES_GET_LIST_SUCCESSFUL|200|Server|
|RES_GET_LIST_FAIL|201|Server|

### 5. ADD FRIEND
|Command|OpCode|Use By|
|---|---|---|
|REQ_ADD_FRIEND|21|Client|

|Command|ResponseCode|Use By|
|---|---|---|
|RES_ADD_FRIEND_SUCCESSFUL|210|Server|
|RES_ADD_FRIEND_FAIL|211|Server|

### 6. SEND CHALLENGE 
|Command|OpCode|Use By|
|---|---|---|
|REQ_SEND_CHALLENGE|22|Client|

|Command|ResponseCode|Use By|
|---|---|---|
|RES_SEND_CHALLENGE_SUCCESSFUL|220|Server|
|RES_SEND_CHALLENGE_FAIL|221|Server|

### 7. STOP MATCH
|Command|OpCode|Use By|
|---|---|---|
|REQ_STOP_MATCH|31|Client|

|Command|ResponseCode|Use By|
|---|---|---|
|RES_STOP_MATCH_SUCCESSFUL|310|
|RES_STOP_MATCH_FAIL|311|

### 8. PLAY
|Command|OpCode|Use By|
|---|---|---|
|REQ_PLAY|32|Client|

|Command|ResponseCode|Use By|
|---|---|---|
|RES_PLAY_SUCCESSFUL|320|
|RES_PLAY_FAIL|321|

### 9. GET LOG
|Command|OpCode|Use By|
|---|---|---|
|REQ_GET_LOG|40|Client|

|Command|ResponseCode|Use By|
|---|---|---|
|RES_GET_LOG_SUCCESSFUL|400|
|RES_GET_LOG_FAIL|401|

### 10. CHAT 
|Command|OpCode|Use By|
|---|---|---|
|REQ_CHAT|80|Client|

|Command|ResponseCode|Use By|
|---|---|---|
|RES_CHAT_SUCCESSFUL|800|
|RES_CHAT_FAIL|801|

### 11. BROADCAST SEND LIST FRIEND 
|Command|OpCode|Use By|
|---|---|---|
|BROADCAST_SEND_LIST|29|Server|

### 12. BROADCAST CHALLENGE
|Command|OpCode|Use By|
|---|---|---|
|BROADCAST_SEND_CHALLENGE|28|Server|

|Command|ResponseCode|Use By|
|---|---|---|
|RES_ACCEPT_CHALLENGE|280|Client|
|RES_DENY_CHALLENGE|281|Client|

### 13. BROADCAST CHALLENGE ACCEPTED DENIED
|Command|OpCode|Use By|
|---|---|---|
|BROADCAST_CHALLENGE_ACCEPTED|22|Server|
|BROADCAST_CHALLENGE_DENIED|23|Server|

### 14. BROADCAST START GAME
|Command|OpCode|Use By|
|---|---|---|
|BROADCAST_START_GAME|30|Server|

### 15. BROADCAST PLAY 
|Command|OpCode|Use By|
|---|---|---|
|BROADCAST_PLAY|89|Server|

### 15. BROADCAST RESULT 
|Command|OpCode|Use By|
|---|---|---|
|BROADCAST_RESULT|89|Server|

### 16. BROADCAST CHAT 
|Command|OpCode|Use By|
|---|---|---|
|BROADCAST_CHAT|89|Server|

# Description
Game có các chức năng:__
1. Đăng nhập
2. Đăng kí
3. Đăng xuất
4. Hiện danh sách người chơi
5. Gửi lời mời kết bạn
6. Chơi game

### Với chức năng ĐĂNG NHẬP: 
1. Client sẽ gửi thông điệp request **REQ_LOGIN**
2. Server xử lý và gửi lại một trong các thông điệp: 
    - **RES_LOGIN_SUCCESSFUL**
    - **RES_LOGIN_FAIL**
    - **RES_LOGIN_FAIL_LOCKED**
    - **RES_LOGIN_FAIL_WRONG_PASS**
    - **RES_LOGIN_FAIL_NOT_EXIST**
    - **RES_LOGIN_FAIL_ANOTHER_DEVICE**
    - **RES_LOGIN_FAIL_ALREADY_LOGIN**
### Với chức năng ĐĂNG KÍ
1. Client sẽ gửi thông điệp request **REQ_REGISTER**
2. Server xử lý và gửi lại một trong các thông điệp:
    - **RES_REGISTER_SUCCESSFUL**
    - **RES_REGISTER_FAIL**
### Với chức năng ĐĂNG XUẤT
1. Client sẽ gửi thông điệp request **REQ_LOGOUT**
2. Server xử lý và gửi lại một trong các thông điệp:
    - **RES_LOGOUT_SUCCESSFUL**
    - **RES_LOGOUT_FAIL**
### Với chức năng HIỆN DANH SÁCH NGƯỜI CHƠI
1. Sau khi đăng nhập thành công, client sẽ gửi thông điệp **REQ_GET_LIST**  server
2. Server xử lý và gửi lại một trong các thông điệp:
    - **RES_GET_LIST_SUCCESSFUL**
    - **RES_GET_LIST_FAIL**
3. Client có thể  lấy danh sách bằng lệnh **BROADCAST_SEND_LIST** từ server
### Với chức năng GỬI LỜI MỜI KẾT BẠN
1. Client có thể yêu cầu kết bạn bằng request **REQ_ADD_FRIEND**
2. Server xử lý và gửi lại một trong các thông điệp:
    - **RES_ADD_FRIEND_SUCCESSFUL**
    - **RES_ADD_FRIEND_FAIL**
### Với chức năng CHƠI GAME
1. Client1 gửi yêu cầu thách đấu đến một đối thủ bằng request **REQ_CHALLENGE**
2. Server sẽ gửi yêu cầu chấp nhận thách đấu đến Client2 bằng request **BROADCAST_SEND_CHALLENGE**
3. Client2 sẽ thông điệp trả lời có hoặc không:
    - **RES_ACCEPCT_CHALLENGE**
    - **RES_DENY_CHALLENGE**
4. Sau đó Server sẽ gửi broadcast đến Client1 với thông điệp tuỳ thuộc vào Client trả lời thế nào:
    - **BROADCAST_CHALLENGE_ACCEPTED**
    - **BROADCAST_CHALLENGE_DENIED**
5. Nếu cả hai đồng ý, Server tiến hành gửi broad cast **BROADCAST_START_GAME**
6. Hai bên tiến hành chơi và gửi các thông điệp **REQ_PLAY**
7. Server sẽ xử lý, gửi lại broad cast cho 2 bên tham gia **BROADCAST_PLAY**
8. Nếu game kết thúc hoặc client gửi yêu cầu dừng game **REQ_STOP_MATCH**, server sẽ gửi kết quả **BROADCAST_RESULT**
