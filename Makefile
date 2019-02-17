
default:
	gcc -o parse_wasm parse_wasm.c -O3 -Wall

clean:
	@rm -f parse_wasm
	@echo "Clean!"

