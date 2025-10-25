# 服务器配置

此目录包含小智 Linux 版本的服务器配置文件。

## server.json

服务器配置文件用于指定小智服务相关的服务器地址信息。

### 配置格式

```json
{
    "server": {
        "hostname": "api.tenclass.net",
        "port": "443",
        "path": "/xiaozhi/v1/",
        "ota_url": "https://api.tenclass.net/xiaozhi/ota/"
    }
}
```

### 配置项说明

- `hostname`: WebSocket 服务器主机名
- `port`: WebSocket 服务器端口
- `path`: WebSocket 连接路径
- `ota_url`: OTA 更新服务器 URL

### 使用方法

1. 修改 `server.json` 文件中的相应配置项
2. 重新编译并运行 `control_center`
3. 程序会自动读取配置文件中的服务器信息

### 注意事项

- 如果配置文件不存在或格式错误，程序将使用内置的默认配置
- 配置文件路径相对于 `control_center` 目录为 `../conf/server.json`