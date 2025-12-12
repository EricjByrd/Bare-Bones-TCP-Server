// TCP_Server.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <stdio.h>
#include <WinSock2.h>
#include <stdint.h>
#define SERVER_ADDRESS 0
#define SERVER_PORT 8081
#pragma comment(lib, "Ws2_32.lib")

void modbusReadRegister(SOCKET clientSocket, char buffer[]) {
    //We've received a request to Read a Register
    //Anatomy of the MbsByteArray[], our initial request from Modbus Master.
    // Byte | Value (hex) | Meaning
    // -----------MBAP Header Starts Here-----------------------------------------------
    // -----|------------|--------------------------------------------
    //  0 | 0x00  | Transaction ID High Byte(any number, identifies request/response)
    //  1 | 0x01  | Transaction ID Low Byte(any number, identifies request/response)
    //  2 | 0x00  | Protocol ID High Byte (0x0000 for Modbus TCP)
    //  3 | 0x00  | Protocol ID Low Byte (0x0000 for Modbus TCP)
    //  4 | 0x00  | Length of remaining bytes High Byte (Unit ID + Function + Data)
    //  5 | 0x06  | Length of remaining bytes Low Byte (Unit ID + Function + Data)                     
    //  6 | 0x01  | Unit ID (Modbus slave address)
    //-------------MBAP Header Stops here-----------------------------------------------
    //  7   | 0x03       | Function Code (0x03 = Read Holding Registers)
    //  8   | 0x00 0x00  | Starting Address High byte
    //  9   | 0x00 0x00  | Starting Address Low byte
    //  10  | 0x00 0x02  | Quantity of Registers High byte
    //  11  | 0x00 0x02  | Quantity of Registers Low byte
    //... additional bytes may follow for other function codes

    //The anatomy of MbData (Slave's response to the Master):
    // Byte | Value (hex) | Meaning
    // -----|------------|--------------------------------------
    // -----------MBAP Header Starts Here-----------------------------------------------
    //  0 | 0x00 0x01  | Transaction ID High Byte(any number, identifies request/response)
    //  1 | 0x00 0x01  | Transaction ID Low Byte(any number, identifies request/response)
    //  2 | 0x00 0x00  | Protocol ID High Byte (0x0000 for Modbus TCP)
    //  3 | 0x00 0x00  | Protocol ID Low Byte (0x0000 for Modbus TCP)
    //  4 | 0x00 0x06  | Length of remaining bytes High Byte (Unit ID + Function + Data)
    //  5 | 0x00 0x06  | Length of remaining bytes Low Byte (Unit ID + Function + Data)                     
    //  6 | 0x01       | Unit ID (Modbus slave address)
    //-------------MBAP Header Stops here-----------------------------------------------
    //  7   | 0x03       | Function Code (0x03 = Read Holding Registers)
    //-------------The data below this is what is overwritten in MbsByteArray starting at index [8] and send to the Master----------------
    //  8   | 0x00 0x00  | Byte Count
    //  9   | ...        | First Register Data High byte
    // 10   | ...        | First Register Data Low byte
    // 11   | ...        | Second Register Data High byte
    // 12   | ...        | Second Register Data Low byte
    // etc.. additional Register Data (if more than 2 registers requested)
        //1. Take an existing request packet (12 bytes)
    uint8_t startAddressHigh = (unsigned char)buffer[8];
    uint8_t startAddressLow = (unsigned char)buffer[9];

    //2. Take bytes 8 and 9 (Starting Address)
    uint16_t startAddress = ((startAddressHigh << 8) | startAddressLow);
    printf("Starting Address: %d\n", startAddress);

    //3. Read the number of registers requested from bytes 10 and 11
    uint8_t numRegistersHigh = (unsigned char)buffer[10];
    uint8_t numRegistersLow = (unsigned char)buffer[11];
    uint16_t numRegisters = ((numRegistersHigh << 8) | numRegistersLow);

    //4. Multiply that value from elements 10 and 11 by 2. Call that the Word size (amount of bytes to read)
    uint16_t byteCount = numRegisters * 2;
    uint16_t numBytes = byteCount + 3;
    uint8_t hsbnumBytes = numBytes >> 8;
    uint8_t lsbnumBytes = numBytes & 0x00FF;
    buffer[4] = hsbnumBytes;
    buffer[5] = lsbnumBytes;
    buffer[8] = byteCount;
    buffer[9] = 0x00; //low byte of first register (empty)
    buffer[10] = 0x03; //low byte of first register (empty)
    buffer[11] = 0x00; //low byte of first register (empty)
    buffer[12] = 0x02; //low byte of second register
    //buffer[13] = 0x00; //low byte of first register (empty)
    //buffer[14] = 0x02; //low byte of third register
    //5. Overwrite the existing request packet from byte 8 onwards with response:
        //a. Byte 8: Byte Count (Word Size from step 4)
        //b. Byte 9: First register's high byte (empty)
        //c. Byte 10: First register's low byte
        //d. etc
    for (int i = 0; i < byteCount + 9; i++) {
        printf("Response Packet: %02X\n", (unsigned char)buffer[i]);
    }
    send(clientSocket, buffer, byteCount + 9, 0);
}

void modbusRequest(SOCKET clientSocket, char buffer[], size_t size)
{
    int bytesReceived = 0;
    int result;

    do {
        //First check to ensure first 6 bytes are received. Then continue receiving until expected size is met.
        while (bytesReceived < 6) {
            //result is = data received on the client.
            //It goes to the buffer, offset by bytes received (0 at first).
            //We expect 6 - bytes received as our expected length each iteration.
            //bytesReceived then becomes bytes received + result so
            //0 becomes 1, 3, 5, etc until we hit 6.
            result = recv(clientSocket, buffer + bytesReceived, 6 - bytesReceived, 0);
            bytesReceived += result;
            for (int i = 0; i < bytesReceived; i++) {
                printf("Bytes received: %02X\n", (unsigned char)buffer[i]);
            }
        }
        uint8_t expectedSizeLow = buffer[5];
        uint8_t expectedSizeHigh = buffer[4];
        uint16_t expectedSize = ((expectedSizeHigh << 8 | expectedSizeLow) + 6);

        printf("Bytes Received: %d\n", bytesReceived);
        printf("Expected size: %d\n", expectedSize);

        //Now continue receiving until expected size is met.
        while (result < expectedSize) {
            printf("Bytes Received: %d\n", bytesReceived);
            printf("Expected size: %d\n", expectedSize);
            //we receive data into buffer + 6.
            //expected size before 12 - 6
            result = recv(clientSocket, buffer + bytesReceived, expectedSize, 0);
            bytesReceived += result;
            printf("Result: %d", result);
            if (result > 0) {
                printf("\nexpected size: %d\n", expectedSize);
                printf("\nexpected size: %d\n", result);
                modbusReadRegister(clientSocket, buffer);
                //just printing buffer for testing purposes
                printf("Message from client: %d\n", result);
                for (int i = 0; i < bytesReceived; i++) {
                    printf("Response :%02X\n", (unsigned char)buffer[i]);
                }
                bytesReceived = 0;
            }
            else if (result == 0) {
                printf("Client disconnected.\n");
                closesocket(clientSocket);
                return;
            }
            else {
                printf("recv failed with error: %d\n", WSAGetLastError());
                closesocket(clientSocket);
                return;
            }
        } bytesReceived = 0;
    } while (result > 0);
}
int main()
{
    //Instantiating the Socket
    WSADATA wsaData;
    WORD wVersionRequested = MAKEWORD(2, 2);
    char dataBuffer[1024] = { 0 };
    int err;
    err = WSAStartup(wVersionRequested, &wsaData);
    if (err != 0) {
        printf("WSAStartup failed with error: %d\n", err);
        return 1;
    }

    printf("////////////////////////////////////////////////////\n"
        "///////Welcome to your Modbus TCP Server//////////////\n"
        "////////////////////////////////////////////////////\n\n");

    printf("Waiting for Modbus client...\n\n");

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    //Building the socket out
    sockaddr_in serverAddress = { SERVER_ADDRESS };

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(SERVER_PORT);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress));
    listen(serverSocket, SOMAXCONN);
    while (1) {
        SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket == INVALID_SOCKET) {
            printf("Connection to Client failed %d\n", WSAGetLastError());
        }
        else {
            printf("Client connected successfully!\n");
            modbusRequest(clientSocket, dataBuffer, sizeof(dataBuffer));
        }
    }
    closesocket(serverSocket);
    WSACleanup();
}