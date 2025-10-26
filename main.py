# https://tobiasvl.github.io/blog/write-a-chip-8-emulator/#specifications

from typing import Iterable, Any
import os

UINT16_MAX = int("0xFFFF", 0)
UINT8_MAX = int("0xFF", 0)

MEMORY_SIZE = 4 * 1024

# Return codes
RC_DECODE_PASS = 0
RC_DECODE_FAIL = -1

class GeneralMemory:
    
    def __init__(self, data_size: int) -> None:
        self.data_size: int = data_size

    def validate(self, memory: Iterable | None = None, address: Any = None, value: int | None = None) -> None:
        '''
        Ghetto vaidation of if the values are the correct size and if the addresses are correct
        '''
        if memory is not None and address is not None and address not in memory:
            raise IOError("Given address out of bounds")
        if value is not None and not(0 <= value <= self.data_size):
            raise IOError(f"Value is out of {int(self.data_size).bit_length()}-bit bounds")

class Registers(GeneralMemory):

    def __init__(self) -> None:
        self.data_size = UINT8_MAX
        self.registers: dict[str, int] = {
            "V0": 0, "V1": 0, "V2": 0, "V3": 0, 
            "V4": 0, "V5": 0, "V6": 0, "V7": 0, 
            "V8": 0, "V9": 0, "VA": 0, "VB": 0, 
            "VC": 0, "VD": 0, "VE": 0, "VF": 0,
        }

    def write_register(self, register: str, value: int) -> None:        
        self.validate(self.registers, register, value)
        self.registers[register] = value

    def read_register(self, register: str) -> int:
        self.validate(self.registers, register)
        return self.registers[register]
    
    def __str__(self) -> str:
        string: list[str] = []
        for i, (key, value) in enumerate(self.registers.items()):
            # Dynamically calculate the padding with 0 for and convert the values to hex
            string.append(f"{key}: {value:0>{int(self.data_size).bit_length()//4}x}")
            string.append("\n") if (i % 4 == 3) else string.append(" | ")

        return "".join(string)    
    
class Memory(GeneralMemory):

    def __init__(self) -> None:
        self.data_size = UINT8_MAX
        self.memory: dict[int, int] = {key: 0 for key in range(MEMORY_SIZE)}
    
    def read_byte(self, address: int) -> int:
        self.validate(self.memory, address)
        return self.memory[address]
    
    def write_byte(self, address: int, value: int) -> None:
        self.validate(self.memory, address, value)
        self.memory[address] = value

    def write_memory_block(self, start: int, values: list[int] | tuple[int]) -> None:
        for i, value in enumerate(values, start=start):
            self.validate(self.memory, i, value)
        for i, value in enumerate(values):
            self.memory[start + i] = value

    def __str__(self) -> str:
        string: list[str] = []
        for i, (key, value) in enumerate(self.memory.items()):
            string.append(f"{key:0>3x}: ") if (i % 16 == 0) else string.append("")
            string.append(f"{value:0>{int(self.data_size).bit_length()//4}x}")
            string.append("\n") if (i % 16 == 15) else string.append(" ")

        return "".join(string)

class Stack(GeneralMemory):

    def __init__(self) -> None:
        self.data_size = UINT16_MAX
        self.__stack: list[int] = []

    @property
    def stack(self) -> list[int]:
        return self.__stack
    
    @stack.setter
    def stack(self, value) -> None:
        self.validate(value=value)
        self.__stack.append(value)
    
    @stack.getter
    def stack(self) -> int:
        if self.is_empty():
            raise IOError("Stack is empty, cannot call pop")
        return self.__stack.pop()
    
    def is_empty(self) -> bool:
        return len(self.__stack) == 0

    def __str__(self) -> str:
        string: list[str] = []
        for i, value in enumerate(self.__stack):
            string.append(f"{i:0>2x}: {value:0>{int(self.data_size).bit_length()//4}x}\n")
        return "".join(string)
        

class Cpu:

    timer_size: int = UINT8_MAX

    def __init__(self) -> None:
        self.pc: int = 0
        self.__delay: int = 0
        self.__sound: int = 0
        self.registers: Registers = Registers()
        self.memory: Memory = Memory()
        self.stack: Stack = Stack() # to assign and get values self.stack.stack must be called

    @classmethod
    def validate_timer(cls, value):
        if not(0 <= value <= cls.timer_size):
            raise IOError(f"Specified values does not fit into the {int(cls.timer_size).bit_length()}-bit size for the timer!")

    @property
    def delay(self) -> int:
        return self.__delay
    
    @delay.getter
    def delay(self) -> int:
        tmp: int = self.__delay
        if tmp > 0:
            self.__delay -= 1
        return tmp
    
    @delay.setter
    def delay(self, value) -> None:
        self.validate_timer(value)
        self.__delay = value
    
    @property
    def sound(self) -> int:
        return self.__sound
    
    @sound.getter
    def sound(self) -> int:
        tmp: int = self.__sound
        if tmp > 0:
            self.__sound -= 1
        return tmp
    
    @sound.setter
    def sound(self, value) -> None:
        self.validate_timer(value)
        self.__sound = value

    def __str__(self) -> str:
        string: list[str] = []
        string.append(f"{self.pc = :0>4x}\n")
        string.append(f"{self.__delay = }\n")
        string.append(f"{self.__sound = }\n")
        string.append("\n")
        string.append(str(self.registers))
        string.append("\n")
        string.append(str(self.memory))
        string.append("\n")
        string.append(str(self.stack))
        return "".join(string)
    
    def decode_instruction(self, opcode: int):
        
        a: int = opcode & int("0xF000", base=0)
        b: int = opcode & int("0x0FFF", base=0)

        match a:
            case 0:
                if b == int("0x00E0", base=0):
                    os.system("cls" if os.name == "nt" else "clear")
                    return_value = RC_DECODE_PASS
                elif b == int("0x00EE", base=0):
                    return_value = RC_DECODE_PASS
                else:
                    return_value = RC_DECODE_FAIL
            case _:
                return_value = RC_DECODE_FAIL

        return return_value


if __name__ == "__main__":
    c = Cpu()
    print()
    