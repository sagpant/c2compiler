package hello_world;

use stdio;

type Number int;

public string name = "Hallo";

int[] numbers = { 1, 1, 2, 3, 5, 8, 13 }

type Buffer struct {
    int size;
    u8* data;
    u16 count;
}

public func void foo(int a, const char* text) {
// char[16] buffer;  doesn't work yet
    sprintf(buffer, "foo %d %s\n", a, text);
}

func int main(int argc, char*[] argv) {
    Buffer buf;

    if (argc == 1) {
        printf("Hello World!\n");
    } else {
        printf("Goodbye!\n");
    }
    return 0;
}
