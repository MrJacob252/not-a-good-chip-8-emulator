# https://tobiasvl.github.io/blog/write-a-chip-8-emulator/#specifications

# TODO Main loop
# TODO Fetch instructions
# TODO Font
# TODO Capture input

from typing import Iterable, Any
import os, re
from random import randint

UINT16_MAX = int("0xFFFF", 0)
UINT8_MAX = int("0xFF", 0)

MEMORY_SIZE = 4 * 1024
NUMBER_OF_REGISTERS = 16
FONT_START_ADDRESS = int("0x50", base=0)

# Return codes
RC_DECODE_PASS = 0
RC_DECODE_FAIL = -1

class GeneralMemory:
    
    def __init__(self, data_size: int) -> None:
        self.data_max: int = data_size

    def validate(self, memory: Iterable | None = None, address: Any = None, value: int | None = None) -> None:
        '''
        Sketchy validation of if the values are the correct size and if the addresses are correct
        '''
        if memory is not None and address is not None and address not in memory:
            raise IOError("Given address out of bounds")
        if value is not None and not(0 <= value <= self.data_max):
            raise IOError(f"Value is out of {int(self.data_max).bit_length()}-bit bounds")

class Registers(GeneralMemory):

    def __init__(self, number_of_regs: int, data_max: int, register_char: str = "V") -> None:
        self.data_max: int = data_max
        self.register_char: str = register_char
        # Maybe the registers should be reworked ot dict[int, int]
        self.registers: dict[int, int] = {key: 0 for key in range(number_of_regs)}

    def write_register(self, register: int, value: int) -> None:        
        self.validate(self.registers, register, value)
        self.registers[register] = value

    def read_register(self, register: int) -> int:
        self.validate(self.registers, register)
        return self.registers[register]
    
    def __str__(self) -> str:
        string: list[str] = []
        for i, (key, value) in enumerate(self.registers.items()):
            # Dynamically calculate the padding with 0 for and convert the values to hex
            string.append(f"{self.register_char}{key:X}: {value:0>{int(self.data_max).bit_length()//4}X}")
            if i != (len(self.registers) - 1):
                string.append("\n") if (i % 4 == 3) else string.append(" | ")

        return "".join(string)    
    
class Memory(GeneralMemory):

    def __init__(self, memory_size: int, data_max: int) -> None:
        self.data_max: int = data_max
        self.memory: dict[int, int] = {key: 0 for key in range(memory_size // int(self.data_max).bit_length())}
    
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
            string.append(f"{key:0>3X}: ") if (i % 16 == 0) else string.append("")
            string.append(f"{value:0>{int(self.data_max).bit_length()//4}x}")
            string.append("\n") if (i % 16 == 15) else string.append(" ")

        return "".join(string)

class Stack(GeneralMemory):

    def __init__(self, data_max: int) -> None:
        self.data_max: int = data_max
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
            string.append(f"{i:0>2x}: {value:0>{int(self.data_max).bit_length()//4}x}\n")
        return "".join(string)
        

class Cpu:

    timer_size: int = UINT8_MAX

    def __init__(self) -> None:
        self.font_address: int = FONT_START_ADDRESS
        self.pc: int = 0
        self.__delay: int = 0
        self.__sound: int = 0
        self.registers: Registers = Registers(number_of_regs=NUMBER_OF_REGISTERS, data_max=UINT8_MAX)
        self.I: Registers = Registers(number_of_regs=1, data_max=UINT16_MAX, register_char="I")
        self.memory: Memory = Memory(memory_size=MEMORY_SIZE, data_max=UINT8_MAX)
        self.stack: Stack = Stack(data_max=UINT16_MAX) # to assign and get values self.stack.stack must be called

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
        # TODO: This should be probably removed
        # Timers should decrease at the rate of 60Hz not after each access
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
        string.append(str(self.I))
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

        # Implement the PC incementation somewhere
        should_increment = True

        match opcode:
            # 0x00E0
            case _ if re.fullmatch(r"00E0", f"{opcode:0>4X}"):
                self.clear_screen()
                return_value = RC_DECODE_PASS
                
            # 0x00EE    
            case _ if re.fullmatch(r"00EE", f"{opcode:0>4X}"):
                self.subroutine_return()
                return_value = RC_DECODE_PASS
                should_increment = False

            # 0x1NNN
            case _ if re.fullmatch(r"1...", f"{opcode:0>4X}"):
                self.flow_jump(opcode=opcode)
                return_value = RC_DECODE_PASS
                should_increment = False
            
            # 0x2NNN
            case _ if re.fullmatch(r"2...", f"{opcode:0>4X}"):
                self.subroutine_call(opcode=opcode)
                return_value = RC_DECODE_PASS
                should_increment = False

            # 0x3XNN
            case _ if re.fullmatch(r"3...", f"{opcode:0>4X}"):
                self.cond_eq(opcode=opcode)
                return_value = RC_DECODE_PASS

            # 0x4XNN
            case _ if re.fullmatch(r"4...", f"{opcode:0>4X}"):
                self.cond_neq(opcode=opcode)
                return_value = RC_DECODE_PASS

            # 0x5XY0
            case _ if re.fullmatch(r"5..0", f"{opcode:0>4X}"):
                self.cond_x_eq_y(opcode=opcode)
                return_value = RC_DECODE_PASS
            
            # 0x6XNN
            case _ if re.fullmatch(r"6...", f"{opcode:0>4X}"):
                return_value = RC_DECODE_PASS
                self.constant_set(opcode=opcode)
            
            # 0x7XNN
            case _ if re.fullmatch(r"7...", f"{opcode:0>4X}"):
                self.constant_add(opcode=opcode)                
                return_value = RC_DECODE_PASS

            # 0x8XY0
            case _ if re.fullmatch(r"8..0", f"{opcode:0>4X}"):
                self.assign_from_register(opcode=opcode)
                return_value = RC_DECODE_PASS

            # 0x8XY1
            case _ if re.fullmatch(r"8..1", f"{opcode:0>4X}"):
                self.bit_or(opcode=opcode)
                return_value = RC_DECODE_PASS

            # 0x8XY2
            case _ if re.fullmatch(r"8..2", f"{opcode:0>4X}"):
                self.bit_and(opcode=opcode)    
                return_value = RC_DECODE_PASS
            
            # 0x8XY3
            case _ if re.fullmatch(r"8..3", f"{opcode:0>4X}"):
                self.bit_xor(opcode=opcode)
                return_value = RC_DECODE_PASS
            
            # 0x8XY4
            case _ if re.fullmatch(r"8..4", f"{opcode:0>4X}"):
                self.math_add(opcode=opcode)
                return_value = RC_DECODE_PASS

            # 0x8XY5
            case _ if re.fullmatch(r"8..5", f"{opcode:0>4X}"):
                self.math_sub_x_y(opcode=opcode)
                return_value = RC_DECODE_PASS
            
            # 0x8XY6
            case _ if re.fullmatch(r"8..6", f"{opcode:0>4X}"):
                self.bit_shift_r(opcode=opcode)
                return_value = RC_DECODE_PASS

            # 0x8XY7
            case _ if re.fullmatch(r"8..7", f"{opcode:0>4X}"):
                self.math_sub_y_x(opcode=opcode)
                return_value = RC_DECODE_PASS

            # 0x8XYE
            case _ if re.fullmatch(r"8..E", f"{opcode:0>4X}"):
                self.bit_shift_l(opcode=opcode)
                return_value = RC_DECODE_PASS

            # 0x9XY0
            case _ if re.fullmatch(r"9..0", f"{opcode:0>4X}"):
                self.cond_x_neq_y(opcode=opcode)
                return_value = RC_DECODE_PASS

            # 0xANNN
            case _ if re.fullmatch(r"A...", f"{opcode:0>4X}"):
                self.mem_set_i(opcode=opcode)
                return_value = RC_DECODE_PASS

            # 0xBNNN
            case _ if re.fullmatch(r"B...", f"{opcode:0>4X}"):
                self.flow_offset_jump(opcode=opcode)
                return_value = RC_DECODE_PASS
                should_increment = False
            
            # 0xCXNN
            case _ if re.fullmatch(r"C...", f"{opcode:0>4X}"):
                self.rand(opcode=opcode)
                return_value = RC_DECODE_PASS

            case _ if re.fullmatch(r"D...", f"{opcode:0>4X}"):
                return_value = RC_DECODE_PASS
                pass

            case _ if re.fullmatch(r"E.9E", f"{opcode:0>4X}"):
                return_value = RC_DECODE_PASS
                pass

            case _ if re.fullmatch(r"E.A1", f"{opcode:0>4X}"):
                return_value = RC_DECODE_PASS
                pass

            # 0xFX07
            case _ if re.fullmatch(r"F.07", f"{opcode:0>4X}"):
                self.timer_get_delay(opcode=opcode)
                return_value = RC_DECODE_PASS

            case _ if re.fullmatch(r"F.0A", f"{opcode:0>4X}"):
                return_value = RC_DECODE_PASS
                pass

            # 0xFX15
            case _ if re.fullmatch(r"F.15", f"{opcode:0>4X}"):
                self.timer_set_delay(opcode=opcode)
                return_value = RC_DECODE_PASS

            # 0xFX18
            case _ if re.fullmatch(r"F.18", f"{opcode:0>4X}"):
                self.timer_set_sound(opcode=opcode)
                return_value = RC_DECODE_PASS

            # 0xFX1E
            case _ if re.fullmatch(r"F.1E", f"{opcode:0>4X}"):
                self.mem_add_to_i(opcode=opcode)
                return_value = RC_DECODE_PASS
            
            # 0xFX29
            case _ if re.fullmatch(r"F.29", f"{opcode:0>4X}"):
                self.mem_sprite_address(opcode=opcode)
                return_value = RC_DECODE_PASS
                pass

            # 0xFX33
            case _ if re.fullmatch(r"F.33", f"{opcode:0>4X}"):
                self.binary_coded_decimal(opcode=opcode)
                return_value = RC_DECODE_PASS

            # 0xFX55
            case _ if re.fullmatch(r"F.55", f"{opcode:0>4X}"):
                self.mem_reg_dump(opcode=opcode)
                return_value = RC_DECODE_PASS

            # 0xFX65
            case _ if re.fullmatch(r"F.65", f"{opcode:0>4X}"):
                self.mem_reg_load(opcode=opcode)
                return_value = RC_DECODE_PASS

            case _:
                return_value = RC_DECODE_FAIL

        return (should_increment, return_value)

    # 0x00E0
    @staticmethod
    def clear_screen() -> None:
        os.system("cls" if os.name == "nt" else "clear")
    
    # 0x00EE
    def subroutine_return(self) -> None:
        # For some reason it does not take the custom getter int consideration :(
        self.pc = self.stack.stack # type: ignore
    
    # 0x1NNN
    def flow_jump(self, opcode: int) -> None:
        address: int = opcode & int("0x0FFF", base=0)
        self.pc = address
        
    # 0x2NNN
    def subroutine_call(self, opcode:int) -> None:
        address: int = opcode & int("0x0FFF", base=0)
        
        self.stack.stack = self.pc
        self.pc = address

    # 0x3XNN
    def cond_eq(self, opcode: int) -> None:
        reg: int = (opcode & int("0x0F00", base=0)) >> 8
        val: int = opcode & int("0x00FF", base=0)
        
        if self.registers.read_register(reg) == val:
            self.pc += 2
    
    # 0x4XNN
    def cond_neq(self, opcode: int) -> None:
        reg: int = (opcode & int("0x0F00", base=0)) >> 8
        val: int = opcode & int("0x00FF", base=0)
        
        if self.registers.read_register(reg) != val:
            self.pc += 2

    # 0x5XY0
    def cond_x_eq_y(self, opcode: int) -> None:
        reg_x: int = (opcode & int("0x0F00", base=0)) >> 8
        reg_y: int = (opcode & int("0x00F0", base=0)) >> 4
        reg_x_val = self.registers.read_register(reg_x)
        reg_y_val = self.registers.read_register(reg_y)
        
        if reg_x_val == reg_y_val:
            self.pc += 2

    # 0x6XNN
    def constant_set(self, opcode: int) -> None:
        reg: int = (opcode & int("0x0F00", base=0)) >> 8
        val: int = opcode & int("0x00FF", base=0)
        
        self.registers.write_register(reg, val)

    # 0x7XNN
    def constant_add(self, opcode: int) -> None:
        reg: int = (opcode & int("0x0F00", base=0)) >> 8
        val: int = opcode & int("0x00FF", base=0)

        curr_val: int = self.registers.read_register(reg)
        # Modulo because overflow
        new_val: int = (curr_val + val) % (self.registers.data_max + 1)
        self.registers.write_register(reg, new_val)
    
    # 0x8XY0 
    def assign_from_register(self, opcode: int) -> None:
        reg_x: int = (opcode & int("0x0F00", base=0)) >> 8
        reg_y: int = (opcode & int("0x00F0", base=0)) >> 4
        
        new_val: int = self.registers.read_register(reg_y)
        self.registers.write_register(reg_x, new_val)
    
    # 0x8XY1
    def bit_or(self, opcode: int) -> None:
        reg_x: int = (opcode & int("0x0F00", base=0)) >> 8
        reg_y: int = (opcode & int("0x00F0", base=0)) >> 4
        
        new_val: int = self.registers.read_register(reg_x) | self.registers.read_register(reg_y)
        self.registers.write_register(reg_x, new_val)

    # 0x8XY2
    def bit_and(self, opcode: int) -> None:
        reg_x: int = (opcode & int("0x0F00", base=0)) >> 8
        reg_y: int = (opcode & int("0x00F0", base=0)) >> 4
        
        new_val: int = self.registers.read_register(reg_x) & self.registers.read_register(reg_y)
        self.registers.write_register(reg_x, new_val)
        
    # 0x8XY3
    def bit_xor(self, opcode: int) -> None:
        reg_x: int = (opcode & int("0x0F00", base=0)) >> 8
        reg_y: int = (opcode & int("0x00F0", base=0)) >> 4
        
        new_val: int = self.registers.read_register(reg_x) ^ self.registers.read_register(reg_y)
        self.registers.write_register(reg_x, new_val)
    
    # 0x8XY4
    def math_add(self, opcode: int) -> None:
        reg_x: int = (opcode & int("0x0F00", base=0)) >> 8
        reg_y: int = (opcode & int("0x00F0", base=0)) >> 4
        
        new_val: int = self.registers.read_register(reg_x) + self.registers.read_register(reg_y)
        self.registers.write_register(reg_x, (new_val & self.registers.data_max))
        

        # Set VF to 1 if overflow
        status = 1 if (new_val > self.registers.data_max) else 0
        self.registers.write_register(15, status)
    
    # 0x8XY4
    def math_sub_x_y(self, opcode: int) -> None:
        '''Subtract VX = VX - VY'''
        reg_x: int = (opcode & int("0x0F00", base=0)) >> 8
        reg_y: int = (opcode & int("0x00F0", base=0)) >> 4
        
        new_val: int = self.registers.read_register(reg_x) - self.registers.read_register(reg_y)
        self.registers.write_register(reg_x, (new_val & self.registers.data_max))
        
        # Set VF to 1 if there is no underflow, otherwise 0
        status = 1 if (0 <= new_val <= self.registers.data_max) else 0
        self.registers.write_register(15, status)
    
    # 0x8XY6
    def bit_shift_r(self, opcode: int) -> None:
        reg_x: int = (opcode & int("0x0F00", base=0)) >> 8
        reg_x_val: int = self.registers.read_register(reg_x)
        
        save_bit: int = reg_x_val & 1
        
        self.registers.write_register(reg_x, (reg_x_val >> 1))
        self.registers.write_register(15, save_bit) 
    
    # 0x8XY7
    def math_sub_y_x(self, opcode: int) -> None:
        '''Subtract VX = VY - VX'''
        reg_x: int = (opcode & int("0x0F00", base=0)) >> 8
        reg_y: int = (opcode & int("0x00F0", base=0)) >> 4
        
        new_val: int = self.registers.read_register(reg_y) - self.registers.read_register(reg_x)
        self.registers.write_register(reg_x, (new_val & self.registers.data_max))
        
        # Set VF to 1 if there is no underflow, otherwise 0
        status = 1 if (0 <= new_val <= self.registers.data_max) else 0
        self.registers.write_register(15, status)
    
    # 0x8XYE
    def bit_shift_l(self, opcode: int) -> None:
        reg_x: int = (opcode & int("0x0F00", base=0)) >> 8
        reg_x_val: int = self.registers.read_register(reg_x)
        
        # Crazy calculation to always get 0x8 and the number of 0 so that it matches the bit width of the registers
        save_bit: int = reg_x_val & int(f"0x8{'0' * ((int(self.registers.data_max).bit_length() // 4) - 1)}", base=0)
        save_bit = save_bit > (self.registers.data_max // 4)
        
        self.registers.write_register(reg_x, (reg_x_val << 1) & self.registers.data_max)
        self.registers.write_register(15, save_bit) 
            
    # 0x9XY0
    def cond_x_neq_y(self, opcode: int) -> None:
        reg_x: int = (opcode & int("0x0F00", base=0)) >> 8
        reg_y: int = (opcode & int("0x00F0", base=0)) >> 4
        reg_x_val = self.registers.read_register(reg_x)
        reg_y_val = self.registers.read_register(reg_y)
        
        if reg_x_val != reg_y_val:
            self.pc += 2
            
    # 0xANNN
    def mem_set_i(self, opcode) -> None:
        val: int = opcode & int("0x0FFF", base=0)
        self.I.write_register(0, val)
    
    # 0xBNNN (TODO: Implement the SUPER-CHIP quirk)
    def flow_offset_jump(self, opcode: int) -> None:
        address: int = opcode & int("0x0FFF", base=0)
        new_pc: int = self.registers.read_register(0) + address
        self.pc = new_pc
        
    # 0xCXNN
    def rand(self, opcode: int) -> None:
        reg: int = (opcode & int("0x0F00", base=0)) >> 8
        val: int = opcode & int("0x00FF", base=0)
        
        new_val = randint(0, self.registers.data_max) & val
        self.registers.write_register(reg, new_val)
        
    # 0xFX07
    def timer_get_delay(self, opcode: int) -> None:
        reg: int = (opcode & int("0x0F00", base=0)) >> 8
        self.registers.write_register(reg, self.delay)
    
    # 0xFX15
    def timer_set_delay(self, opcode: int) -> None:
        reg: int = (opcode & int("0x0F00", base=0)) >> 8
        self.delay = self.registers.read_register(reg)
    
    # 0xFX18
    def timer_set_sound(self, opcode: int) -> None:
        reg: int = (opcode & int("0x0F00", base=0)) >> 8
        self.sound = self.registers.read_register(reg)
    
    # 0xFX1E
    def mem_add_to_i(self, opcode: int) -> None:
        reg: int = (opcode & int("0x0F00", base=0)) >> 8
        new_val = self.registers.read_register(reg) + self.I.read_register(0)
        self.I.write_register(reg, new_val)

    # 0xFX29
    def mem_sprite_address(self, opcode: int) -> None:
        reg: int = (opcode & int("0x0F00", base=0)) >> 8

        new_i: int = self.font_address + self.registers.read_register(reg)
        self.I.write_register(0, new_i)

    # 0xFX33 
    def binary_coded_decimal(self, opcode: int) -> None:
        reg: int = (opcode & int("0x0F00", base=0)) >> 8
        val: int = self.registers.read_register(reg)

        # This will be hardcoded for 8-bit registers only 
        # Rework may come later
        digit_list: list[int] = [int(x) for x in f"{val:0>3}"]
        init_address: int = self.I.read_register(0)
        for i, digit in enumerate(digit_list):
            self.memory.write_byte(init_address + i, digit)

    # 0xFX55
    def mem_reg_dump(self, opcode: int) -> None:
        reg: int = (opcode & int("0x0F00", base=0)) >> 8
        init_address: int = self.I.read_register(0)
        
        # Writing out of bounds is guarded by the .write_register() and .read_register() methods
        for i in range(0, reg + 1):
            val_to_save: int = self.registers.read_register(i)
            self.memory.write_byte(init_address + i, val_to_save)
    
    # 0xFX65
    def mem_reg_load(self, opcode: int) -> None:
        reg: int = (opcode & int("0x0F00", base=0)) >> 8
        init_address = self.I.read_register(0)
        
        # Writing out of bounds is guarded by the .write_register() and .read_register() methods
        for i in range(0, reg + 1):
            val_to_load: int = self.memory.read_byte(init_address + i)
            self.registers.write_register(i, val_to_load)
    
        
        

if __name__ == "__main__":
    c = Cpu()
    print()
    
    def debug_clear_regs(m: int) -> None:
        for i in range(0, m + 1):
            c.decode_instruction(int(f"0x6{i:X}00", base=0))
    
    def debug_fill_regs(m: int) -> None:
        for i in range(0, m + 1):
            c.decode_instruction(int(f"0x6{i:X}{(i+1):0>2X}", base=0))    
    
    def debug_decode_regs(op_string: str) -> None:
        opcode = int(op_string, base=0)
        inc, ret_code = c.decode_instruction(opcode)
        color = 32 if (ret_code == 0) else 31
        print(f"\x1b[{color}m{ret_code}\x1b[0m")
        if inc:
            c.pc += 2
        print(f"{c.pc = :X}")
        print(str(c.registers))
        print(str(c.I))
        print(str(c.stack))
    