CFLAGS =
DEBUG = 0

ifeq ($(DEBUG), 1)
    CFLAGS += -g
endif

all: simple_shell testsh

simple_shell: 2024_lab2_shellwithTODO.c
	gcc $(CFLAGS) -o simple_shell 2024_lab2_shellwithTODO.c

# TODO: 参照 simple_shell，增加编译 testsh 的规则
testsh: testsh.c
	gcc $(CFLAGS) -o testsh testsh.c

# TODO: 添加自动化测试的lable
.PHONY : clean test debug

clean:
	rm -f testsh simple_shell

# TODO: 参照 clean ，增加自动化测试的规则，注意自动化测试的规则是可以有依赖的
test: testsh simple_shell
	./testsh simple_shell