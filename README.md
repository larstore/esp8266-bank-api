# esp8266-bank-api
A complete solution to get account information using the sbanken API. Should be easily configurable to other bank apis i guess

Based on this idea https://github.com/oyvindt/sbanken-saldogris

1. Use the ESP2866 httpclient also found here to enable bearer token auth(will try to get this merged)
2. Program one of these (not tested this exact one)https://www.ebay.com/itm/NodeMcu-Lua-V2-WIFI-Internet-Thing-Development-Board-Based-ESP8266-CP2102-Module/181924392878?epid=16012781503&hash=item2a5b89efae:g:Uf0AAOSwepJXT5ms
3. Connect one of these (not tested this exact one) https://www.ebay.com/itm/1-3-White-OLED-LCD-4PIN-Display-Module-IIC-I2C-Interface-128x64-for-Arduino/401395109681?hash=item5d75034f31:g:WH4AAOSwVGFZqEDx

4. The device will show up as a wifi access point. Configure a wifi and add apikey,password,userid and accountname.
5. The device will reset and show the available funds on the connected screen. Refreshed every 15 seconds