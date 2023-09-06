# Webserv
- hyeonjan
- gychoi

## git merge
1. 내 작업 공간에 최신 develop 내용을 반영한다. (git fetch -> git pull -> git merge develop)
2. 내 작업 공간에서 작업 수행 후 커밋하고 원격 저장소로 push한다. (git add . & git commit -> git push)
3. 원격 저장소에서 팀원들의 허락을 받아 PR을 마무리한다. (PR -> { git checkout develop -> git merge })
	- develop으로 이동 후, 내 작업 공간 내용을 반영한다. (git checkout develop -> git merge)
4. 로컬의 develop 브랜치를 최신화하고, 내 작업 공간을 시작한다. (git fetch -> git pull -> git checkout my_branch)
5. WOW!
