# Seeed RTL87XX AT firmware [![Build Status](https://travis-ci.com/Seeed-Studio/ambd-sdk.svg?branch=AT)](https://travis-ci.com/github/Seeed-Studio/seeed-ambd-sdk)

## Introduction

This RTL87XX AT firmware export a AT command interface through hardware SPI/UART port to MCU.  
But current only works for RTL8720DN/RTL8720DM.

With those AT commands, the MCU could scan/connect WiFi APs, connect Internet/Intranet through the APs,
or act as a TCP server (Web Server, Telnet Server, etc).


## How to compile on Ubuntu Bionic
- Clone this repo

```shell
    cd; git clone -b AT https://github.com/Seeed-Studio/seeed-ambd-sdk.git
```
- Prepare gcc compiler

```shell
    cd seeed-ambd-sdk
    GCC_URL="http://files.seeedstudio.com/arduino/tools/arm-none-eabi/asdk-6.4.1-linux-newlib-build-2773-i686.tar.bz2"
    GCC_TOOL="project/realtek_amebaD_va0_example/GCC-RELEASE/project_hp/toolchain/asdk/$(basename $GCC_URL)"
    curl -fsSL $GCC_URL -o $GCC_TOOL

    # This gcc is x86 version, for X86_64 machine, install package libc6-i386
    sudo apt update
    sudo apt install libc6-i386
```

- Start compiling

```shell
    make -C project/realtek_amebaD_va0_example/GCC-RELEASE/project_lp
    make -C project/realtek_amebaD_va0_example/GCC-RELEASE/project_hp CONFIG_WIFI_COUNTRY=RTW_COUNTRY_US CONFIG_WIFI_CHANNEL_PLAN=0x27
```

|Country|CONFIG_WIFI_COUNTRY|CONFIG_WIFI_CHANNEL_PLAN|
|:--|:--|:--|
|United States|RTW_COUNTRY_US|0x27|
|Japan|RTW_COUNTRY_JP|0x27|

- Check images compiled  
Three binary files (km0_boot_all.bin, km4_boot_all.bin, km0_km4_image2.bin) must be exist if compiling successful.

```shell
    ls -l project/realtek_amebaD_va0_example/GCC-RELEASE/project_lp/asdk/image/km0_boot_all.bin
    ls -l project/realtek_amebaD_va0_example/GCC-RELEASE/project_hp/asdk/image/km4_boot_all.bin
    ls -l project/realtek_amebaD_va0_example/GCC-RELEASE/project_hp/asdk/image/km0_km4_image2.bin
```

## How to burn images to RTL87XX chip

Refer to [Wio-Terminal-Network-Overview](https://wiki.seeedstudio.com/Wio-Terminal-Network-Overview)
to update to Wio Terminal.  
Or Step 'Updating the Firmware' in above page to update to other RTL8720D device.

## AT command list

<div>
  <table border="0">
    <tr align="center">
      <th>WiFi/TCP Command</th>
      <th>Format</th>
      <th>Argument/Comment</th>
      <th>Response</th>
    </tr>
    <tr align="center">
      <td>AT</td>
      <td>AT</td>
      <td>a dummy command only, could used to get OK response<br>
      </td>
      <td>OK</td>
    </tr>
    <tr align="center">
      <td>ATE</td>
      <td>ATE0/ATE1</td>
      <td>ATE0 disable command echo<br>
          ATE1 enable command echo<br>
          <B>unimplement</B>
      </td>
      <td>OK</td>
    </tr>
    <tr align="center">
      <td>AT+GMR</td>
      <td>AT+GMR</td>
      <td>Query firmware version</td>
      <td>OK</td>
    </tr>
    <tr align="center">
      <td>AT+GPIO</td>
      <td>Query: AT+GPIO=R,&lt;port-pin&gt;[,0[,pull-set]]<br>
          Set:   AT+GPIO=W,&lt;port-pin&gt;,&lt;value&gt;[,pull-set]
      </td>
      <td>Query or set GPIO level<br>
      &lt;port-pin&gt: PA25 if port is A,  pin is 25<br>
      &lt;value&gt;: could be 0(low) or 1(high)
      </td>
      <td>OK</td>
    </tr>
    <tr align="center">
      <td>AT+RST</td>
      <td>AT+RST</td>
      <td>Reboot the RTL87XX device</td>
      <td>OK</td>
    </tr>
    <tr align="center">
      <td>AT+CWMODE</td>
      <td>AT+CWMODE</td>
      <td>Query or set Wireless mode</td>
      <td>OK</td>
    </tr>
    <tr align="center">
      <td>AT+CWDHCP</td>
      <td>AT+CWDHCP</td>
      <td>Query or set DHCP mode</td>
      <td>OK</td>
    </tr>
    <tr align="center">
      <td>AT+CWLAP</td>
      <td>AT+CWLAP</td>
      <td>List all scaned AP(Access Points)</td>
      <td>OK</td>
    </tr>
    <tr align="center">
      <td>AT+CWQAP</td>
      <td>AT+CWQAP</td>
      <td>Quit connection to AP(Access Points)</td>
      <td>OK</td>
    </tr>
    <tr align="center">
      <td>AT+CWJAP</td>
      <td>AT+CWJAP</td>
      <td>Get or set the connection to AP(Access Points)</td>
      <td>OK</td>
    </tr>
    <tr align="center">
      <td>AT+CWSAP</td>
      <td>AT+CWSAP</td>
      <td>Get or set this AP(AP in RTL87XX)</td>
      <td>OK</td>
    </tr>
    <tr align="center">
      <td>AT+CIPSTATUS</td>
      <td>AT+CIPSTATUS</td>
      <td>Query TCP/UDP connection status</td>
      <td>OK</td>
    </tr>
    <tr align="center">
      <td>AT+CIPSTA</td>
      <td>AT+CIPSTA</td>
      <td>Query or set station IP address</td>
      <td>OK</td>
    </tr>
    <tr align="center">
      <td>AT+CIPSTAMAC</td>
      <td>AT+CIPSTAMAC</td>
      <td>Query or set station MAC address</td>
      <td>OK</td>
    </tr>
    <tr align="center">
      <td>AT+CIPAP</td>
      <td>AT+CIPAP</td>
      <td>Query or set IP address of this AP</td>
      <td>OK</td>
    </tr>
    <tr align="center">
      <td>AT+CIPAPMAC</td>
      <td>AT+CIPAPMAC</td>
      <td>Query or set MAC address of this AP</td>
      <td>OK</td>
    </tr>
    <tr align="center">
      <td>AT+CIPDOMAIN</td>
      <td>AT+CIPDOMAIN</td>
      <td>Query IP address of a domain name (DNS function)</td>
      <td>OK</td>
    </tr>
    <tr align="center">
      <td>AT+CIPSTART</td>
      <td>AT+CIPSTART</td>
      <td>Create a TCP/UDP connection</td>
      <td>OK</td>
    </tr>
    <tr align="center">
      <td>AT+CIPSEND</td>
      <td>AT+CIPSEND</td>
      <td>Send data to specific connection peer</td>
      <td>OK</td>
    </tr>
    <tr align="center">
      <td>AT+CIPCLOSE</td>
      <td>AT+CIPCLOSE</td>
      <td>Close a specific connection</td>
      <td>OK</td>
    </tr>
    <tr align="center">
      <td>AT+CIPDNS</td>
      <td>AT+CIPDNS</td>
      <td>Query or set the DNS address for RTL87XX</td>
      <td>OK</td>
    </tr>
    <tr align="center">
      <td>AT+CIPSERVER</td>
      <td>AT+CIPSERVER</td>
      <td>Create a TCP/UDP Server to accept connection from Client</td>
      <td>OK</td>
    </tr>
  </table>

  <table border="0">
    <tr align="center">
      <th>BLE Command</th>
      <th>Format</th>
      <th>Argument/Comment</th>
      <th>Response</th>
    </tr>
    <tr align="center">
      <td>ATBp</td>
      <td>ATBp=&lt;start-stop&gt;</td>
      <td>Start/stop BLE peripheral<br>
      &lt;start-stop&gt; could be 0(stop) or 1(start)
      <br>
      </td>
      <td>OK</td>
    </tr>
    <tr align="center">
      <td>ATBc</td>
      <td>ATBc</td>
      <td>Start/stop BLE Central</td>
      <td>OK</td>
    </tr>
    <tr align="center">
      <td>ATBS</td>
      <td>ATBS</td>
      <td>Scan BT</td>
      <td>OK</td>
    </tr>
    <tr align="center">
      <td>ATBC</td>
      <td>ATBC</td>
      <td>Create a GATT connection(connect to remote device)</td>
      <td>OK</td>
    </tr>
    <tr align="center">
      <td>ATBD</td>
      <td>ATBD</td>
      <td>Disconnect from remote device</td>
      <td>OK</td>
    </tr>
    <tr align="center">
      <td>ATBK</td>
      <td>ATBK</td>
      <td>Reply GAP passkey, config authentication mode</td>
      <td>OK</td>
    </tr>
    <tr align="center">
      <td>ATBG</td>
      <td>ATBG</td>
      <td>Get peripheral information</td>
      <td>OK</td>
    </tr>
    <tr align="center">
      <td>ATBI</td>
      <td>ATBI</td>
      <td>Get information of connected device</td>
      <td>OK</td>
    </tr>
    <tr align="center">
      <td>ATBY</td>
      <td>ATBY</td>
      <td>Reply GAP user confirm</td>
      <td>OK</td>
    </tr>
    <tr align="center">
      <td>ATBU</td>
      <td>ATBU</td>
      <td>Update connection request</td>
      <td>OK</td>
    </tr>
    <tr align="center">
      <td>ATBO</td>
      <td>ATBO</td>
      <td>Get/clear bond information</td>
      <td>OK</td>
    </tr>
    <tr align="center">
      <td>ATBR</td>
      <td>ATBR</td>
      <td>GATT client read</td>
      <td>OK</td>
    </tr>
    <tr align="center">
      <td>ATBW</td>
      <td>ATBW</td>
      <td>GATT client write</td>
      <td>OK</td>
    </tr>
    <tr align="center">
      <td>ATBB</td>
      <td>ATBB</td>
      <td>Start/stop BT config</td>
      <td>OK</td>
    </tr>
    <tr align="center">
      <td>ATBJ</td>
      <td>ATBJ</td>
      <td>Start/stop BT Beacon</td>
      <td>OK</td>
    </tr>
    <tr align="center">
      <td>ATBb</td>
      <td>ATBb</td>
      <td>Start/stop BT Airsync config</td>
      <td>OK</td>
    </tr>
    <tr align="center">
      <td>ATBf</td>
      <td>ATBf</td>
      <td>Start/stop BLE Scatternet</td>
      <td>OK</td>
    </tr>
  </table>
</div>

----

This software AT command section is written by Seeed Studio<br>
and is licensed under [The MIT License](http://opensource.org/licenses/mit-license.php). Check License.txt for more information.<br>

Contributing to this software is warmly welcomed. You can do this basically by<br>
[forking](https://help.github.com/articles/fork-a-repo), committing modifications and then [pulling requests](https://help.github.com/articles/using-pull-requests) (follow the links above<br>
for operating guide). Adding change log and your contact into file header is encouraged.<br>
Thanks for your contribution.

Seeed Studio is an open hardware facilitation company based in Shenzhen, China. <br>
Benefiting from local manufacture power and convenient global logistic system, <br>
we integrate resources to serve new era of innovation. Seeed also works with <br>
global distributors and partners to push open hardware movement.<br>
