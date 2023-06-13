# pintOS
Brand new pintos for Operating Systems and Lab (CS330), KAIST, by Youngjin Kwon.

The manual is available at https://casys-kaist.github.io/pintos-kaist/.


# Environment
`ubuntu 18.04 version` <br>

## EC2 setting
```
$ sudo apt update
$ sudo apt install -y gcc make qemu-system-x86 python3
```

## pintOS repository 복제
```
$ git clone --bare https://github.com/casys-kaist/pintos-kaist.git
$ cd pintos-kaist.git
$ git push --mirror https://github.com/${교육생 or 팀ID}/pintos-kaist.git
$ cd ..
$ rm -rf pintos-kaist.git
$ git clone https://github.com/${교육생ID}/pintos-kaist.git
```

## Test pintOS
```
$ cd pintos-kaist
$ source ./activate
$ cd threads
$ make check
# 뭔가 한참 compile하고 test 프로그램이 돈 후에 다음 message가 나오면 정상
20 of 27 tests failed.
```


# Tool install 

## Gitmoji

> Window

```
$ npm i -g gitmoji-cli
``` 

<br>

> Mac OS


```
$ brew install gitmoji
```

# Commit Rule

## gitmoji

📝 (code : `:memo:`) : 파일 및 코드 추가 ex) .gitignore <br>
✅ (code : `:white_check_mark:`) : 프로젝트, 기능 완성한 파일 ex) project1 thread <br>
🐛 (code : `:bug:`) : 버그 발생시 수정한 파일 ex) failed test, else.. <br>
✏️ (code : `:pencil2:`) : 오타 및 주석 추가 및 수정 <br>
🔥 (code : `:fire:`) : 파일 및 코드 삭제

> 여기서 입력한 `origin`은 origin/main 브런치로 깃허브 상에서 default 브런치를 가리킨다.
> git push, pull 등 명령어 수행 시 브런치 명 위치 확실히 생각하고 쓸 것.

## commit, push

> 로컬 브런치 명은 깃허브 아이디로 하고, origin에 푸시한 후 PR한 후 merge를 시도한다.

```
git add file.name
or
git add .
gitmoji -c
1. vscode 상에서 gitmoji 선택
2. commit title : add, delete 등 먼저 명확한 의미 작성 ex) add .gitignore
3. commit message : 상세 내용 ex) .gitignore 불필요한 .vscode 폴더 추가
git push origin 내 브런치 명 ex) git push origin uujeen
```

## merge
```
push 후 깃헙 페이지에서 PR 후 팀원들과 상의 후 merge하기
```

## pull

```
git pull origin 내 브런치 명 ex) git pull origin uujeen
```

# Run

## build_launch.sh
pintOS 초기 폴더에서 `sudo chmod 755 build_launch.sh` 로 권한을 설정한 뒤에 `$ ./build_launch.sh` 로 파일 실행