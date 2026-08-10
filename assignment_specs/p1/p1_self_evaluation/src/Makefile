all: clean dep linux64 linuxarm64 macos macosarm
	go build -ldflags="-s -w"

dep:
	mkdir -p bin

linux64:
	env CGO_ENABLED=0 GOOS=linux GOARCH=amd64 go build -o bin/marking-linux-amd64 -ldflags="-s -w"

linuxarm64:
	env CGO_ENABLED=0 GOOS=linux GOARCH=arm64 go build -o bin/marking-linux-arm64 -ldflags="-s -w"

macos:
	env CGO_ENABLED=0 GOOS=darwin GOARCH=amd64 go build -o bin/marking-macos-amd64 -ldflags="-s -w"

macosarm:
	env CGO_ENABLED=0 GOOS=darwin GOARCH=arm64 go build -o bin/marking-macos-arm -ldflags="-s -w"

clean:
	rm -rf bin
