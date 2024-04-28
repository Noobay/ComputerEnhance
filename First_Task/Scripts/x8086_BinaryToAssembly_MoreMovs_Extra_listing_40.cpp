#include <iostream>

// 8 bits - [100010,(d)x,(w)x]
const uint8_t COMMAND_MOVE_REGISTER_MEM_TO_FROM_REGISTER = 0b100010;
// 8 bits - [1100011, (w)x]
const uint8_t COMMAND_MOVE_IMMEDIATE_TO_REGISTER_MEM = 0b1100011;
// 8 bits - [1010000, (w)x]
const uint8_t COMMAND_MOVE_MEMORY_TO_ACCUMULATOR = 0b1010000;
// 8 bits - [1010001, (w)x]
const uint8_t COMMAND_MOVE_ACCUMULATOR_TO_MEMORY = 0b1010001;
// 8 bits - [1011,(w)x,(reg)xxx]
const uint8_t COMMAND_MOVE_IMMEDIATE_MEM_TO_REGISTER = 0b1011;

const int BUFF_SIZE = 64;

const int MAX_16BIT_DIGIT_COUNT = 5; // (worst case - uint16) 2^16 => 65536
const int MAX_DISPLACEMENT_TEXT_SIZE = MAX_16BIT_DIGIT_COUNT + 2; // [16_BIT_DIGIT]

const uint16_t EXTEND_SIGN_MASK = (0b11111111 << 8);

const char SIZE_DESCRIPTOR_BYTE[] = "byte";
const char SIZE_DESCRIPTOR_WORD[] = "word";

void getRegName(char source[3], uint8_t mode, uint8_t word);
void getDisplacementText(int16_t value, char text[MAX_DISPLACEMENT_TEXT_SIZE]);
int getRegisterOrMemoryText(char outTextBuff[BUFF_SIZE], char tmpDisplacementTextBuff[MAX_DISPLACEMENT_TEXT_SIZE], uint8_t* dataBuff, int dataBuffSize, int activeDataBuffIdx, uint8_t word, uint8_t mode, uint8_t reg_mem);

int main(int argumentsCount, char** argumentsValue)
{
    if (argumentsCount <= 1)
    {
        printf("No Arguments were provided, please provide a path to a binary file");
        return 1;
    }

    char* filePath = argumentsValue[1];

    FILE* pFile;
    errno_t error = fopen_s(&pFile, filePath, "r");
    if (error != 0)
        printf("Error opening file");

    uint8_t fileData[BUFF_SIZE];

    if (pFile != 0)
    {
        int fileBytesCount = fread_s(fileData, BUFF_SIZE, 1, BUFF_SIZE, pFile);

        printf("bits 16\n");

        char registerName[3];
        char registerOrMemoryAddrText[BUFF_SIZE];
        char displacementText[MAX_DISPLACEMENT_TEXT_SIZE];

        registerName[2] = '\0';
        registerOrMemoryAddrText[2] = '\0';

        // Assuming Little Endian
        int byteOffset = 0;
        while (byteOffset < fileBytesCount)
        {
            uint8_t commandByte1 = fileData[byteOffset++];
	        if ((commandByte1 >> 1) == COMMAND_MOVE_MEMORY_TO_ACCUMULATOR)
            {
	        	uint8_t word = commandByte1 & 0b1;
                uint16_t data = fileData[byteOffset++];
		        if (word == 1)
                    data |= (fileData[byteOffset++] << 8);

                printf("mov ax, [%u]\n", data);

            }
            else if ((commandByte1 >> 1) == COMMAND_MOVE_ACCUMULATOR_TO_MEMORY)
            {
                uint8_t word = commandByte1 & 0b1;
                uint16_t data = fileData[byteOffset++];
		        if (word == 1)
                    data |= (fileData[byteOffset++] << 8);

                printf("mov [%u], ax\n", data);
            }
            else if ((commandByte1 >> 1) == COMMAND_MOVE_IMMEDIATE_TO_REGISTER_MEM)
            {
                uint8_t word = commandByte1 & 0b1;

                uint8_t commandByte2 = fileData[byteOffset++];

                uint8_t mode = (commandByte2 >> 6) & 0b11;
                uint8_t reg_mem = commandByte2 & 0b111;

                getRegisterOrMemoryText(registerOrMemoryAddrText, displacementText, fileData, fileBytesCount, byteOffset, 1, mode, reg_mem);

                uint16_t data = fileData[byteOffset++];

                const char* sizeDescriptor = SIZE_DESCRIPTOR_BYTE;
                if (word == 1)
                {
                    data |= (fileData[byteOffset++] << 8);
                    sizeDescriptor = SIZE_DESCRIPTOR_WORD;
                }

                printf("mov %s, %s %u\n", registerOrMemoryAddrText, sizeDescriptor, data);
            }
            else if ((commandByte1 >> 2) == COMMAND_MOVE_REGISTER_MEM_TO_FROM_REGISTER)
            {
                uint8_t direction = (commandByte1 >> 1) & 0b1;
                uint8_t word = commandByte1 & 0b1;

                uint8_t commandByte2 = fileData[byteOffset++];
                uint8_t mode = (commandByte2 >> 6) & 0b11;
                uint8_t reg = (commandByte2 >> 3) & 0b111;
                uint8_t reg_mem = commandByte2 & 0b111;

                getRegName(registerName, reg, word);
                getRegisterOrMemoryText(registerOrMemoryAddrText, displacementText, fileData, fileBytesCount, byteOffset, word, mode, reg_mem);

                if (direction == 1)
                    printf("mov %s, %s", registerName, registerOrMemoryAddrText);
                else
                    printf("mov %s, %s", registerOrMemoryAddrText, registerName);

                printf("\n");
            }
            else if ((commandByte1 >> 4) == COMMAND_MOVE_IMMEDIATE_MEM_TO_REGISTER)
            {
                uint8_t word = (commandByte1 >> 3) & 0b1;
                uint8_t reg = commandByte1 & 0b111;
                uint16_t data = fileData[byteOffset++];

                getRegName(registerName, reg, word);
                if (word == 1)
                    data |= (fileData[byteOffset++] << 8);

                printf("mov %s, %u\n", registerName, data);
            }
        }
    }

    return 0;

}

inline int getRegisterOrMemoryText(char outTextBuff[BUFF_SIZE], char tmpDisplacementTextBuff[MAX_DISPLACEMENT_TEXT_SIZE], uint8_t* dataBuff, int dataBuffSize, int activeDataBuffIdx, uint8_t word, uint8_t mode, uint8_t reg_mem)
{
    switch (mode)
    {
    case 0b11:
        getRegName(outTextBuff, reg_mem, word);
        break;
    case 0b00:
        switch (reg_mem)
        {
        case 0b000:
            memcpy(outTextBuff, "[bx + si]\0", sizeof(char) * 10);
            break;
        case 0b001:
            memcpy(outTextBuff, "[bx + di]\0", sizeof(char) * 10);
            break;
        case 0b010:
            memcpy(outTextBuff, "[bp + si]\0", sizeof(char) * 10);
            break;
        case 0b011:
            memcpy(outTextBuff, "[bp + di]\0", sizeof(char) * 10);
            break;
        case 0b100:
            memcpy(outTextBuff, "si\0", sizeof(char) * 3);
            break;
        case 0b101:
            memcpy(outTextBuff, "di\0", sizeof(char) * 3);
            break;
        case 0b111:
            memcpy(outTextBuff, "bx\0", sizeof(char) * 3);
            break;
        case 0b110:
            int nextDataBuffIdx = activeDataBuffIdx + 2;
            if (nextDataBuffIdx >= dataBuffSize)
            {
                char errorText[128];
                snprintf(errorText, 128, "dataBuff access out of bounds for [Active Idx ~ %i Next Idx ~ %i]", activeDataBuffIdx, nextDataBuffIdx);
                throw std::out_of_range(errorText);
            }
            
            uint8_t lowDisp = dataBuff[activeDataBuffIdx++];
            uint8_t highDisp = dataBuff[activeDataBuffIdx++];
            uint16_t value = (highDisp << 8) | lowDisp;
            snprintf(outTextBuff, BUFF_SIZE, "[%u]", value);

            // Special Case DIRECT ADDRESS 
            break;
        }
        break;
    case 0b01:
    case 0b10:
    {
        int nextDataBuffIdx = activeDataBuffIdx + 1;
        if (mode == 0b10)
            ++nextDataBuffIdx;

        if (nextDataBuffIdx >= dataBuffSize)
        {
            char errorText[128];
            snprintf(errorText, 128, "dataBuff access out of bounds for [Active Idx ~ %i Next Idx ~ %i]", activeDataBuffIdx, nextDataBuffIdx);
            throw std::out_of_range(errorText);
        }

        int16_t data = dataBuff[activeDataBuffIdx++];
        if (mode == 0b10)
            data |= (dataBuff[activeDataBuffIdx++] << 8);
        else
        {
            uint8_t sign = data >> 7;
            if (sign == 1)
                data |= EXTEND_SIGN_MASK;
        }

        getDisplacementText(data, tmpDisplacementTextBuff);

        switch (reg_mem)
        {
            case 0b000:
                snprintf(outTextBuff, BUFF_SIZE, "[bx + si %s]", tmpDisplacementTextBuff);
                break;
            case 0b001:
                snprintf(outTextBuff, BUFF_SIZE, "[bx + di %s]", tmpDisplacementTextBuff);
                break;
            case 0b010:
                snprintf(outTextBuff, BUFF_SIZE, "[bp + si %s]", tmpDisplacementTextBuff);
                break;
            case 0b011:
                snprintf(outTextBuff, BUFF_SIZE, "[bp + di %s]", tmpDisplacementTextBuff);
                break;
            case 0b100:
                snprintf(outTextBuff, BUFF_SIZE, "[si %s]", tmpDisplacementTextBuff);
                break;
            case 0b101:
                snprintf(outTextBuff, BUFF_SIZE, "[di %s]", tmpDisplacementTextBuff);
                break;
            case 0b110:
                snprintf(outTextBuff, BUFF_SIZE, "[bp %s]", tmpDisplacementTextBuff);
                break;
            case 0b111:
                snprintf(outTextBuff, BUFF_SIZE, "[bx %s]", tmpDisplacementTextBuff);
                break;
        }

        break;
    }
    }

    return activeDataBuffIdx;
}

inline void getDisplacementText(int16_t value, char text[MAX_DISPLACEMENT_TEXT_SIZE])
{
    if (value >= 0)
        snprintf(text, MAX_DISPLACEMENT_TEXT_SIZE, "+ %i", value);
    else
    {
        int16_t positiveValue = ~value + 1;
        snprintf(text, MAX_DISPLACEMENT_TEXT_SIZE, "- %i", positiveValue);
    }
}

inline void getRegName(char buff[3], uint8_t reg, uint8_t word)
{
    if (word == 0)
    {
        switch (reg)
        {
            case 0b000:
                buff[0] = 'a';
                buff[1] = 'l';
                break;
            case 0b001:
                buff[0] = 'c';
                buff[1] = 'l';
                break;
            case 0b010:
                buff[0] = 'd';
                buff[1] = 'l';
                break;
            case 0b011:
                buff[0] = 'b';
                buff[1] = 'l';
                break;
            case 0b100:
                buff[0] = 'a';
                buff[1] = 'h';
                break;
            case 0b101:
                buff[0] = 'c';
                buff[1] = 'h';
                break;
            case 0b110:
                buff[0] = 'd';
                buff[1] = 'h';
                break;
            case 0b111:
                buff[0] = 'b';
                buff[1] = 'h';
                break;
        }
    }
    else
    {
        switch (reg)
        {
            case 0b000:
                buff[0] = 'a';
                buff[1] = 'x';
                break;
            case 0b001:
                buff[0] = 'c';
                buff[1] = 'x';
                break;
            case 0b010:
                buff[0] = 'd';
                buff[1] = 'x';
                break;
            case 0b011:
                buff[0] = 'b';
                buff[1] = 'x';
                break;
            case 0b100:
                buff[0] = 's';
                buff[1] = 'p';
                break;
            case 0b101:
                buff[0] = 'b';
                buff[1] = 'p';
                break;
            case 0b110:
                buff[0] = 's';
                buff[1] = 'i';
                break;
            case 0b111:
                buff[0] = 'd';
                buff[1] = 'i';
                break;
        }
    }
}
