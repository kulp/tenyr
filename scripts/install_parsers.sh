case $TRAVIS_OS_NAME in
    osx)
        brew install bison
        BISONS=(/usr/local/Cellar/bison/*/bin/bison)
        export YACC="${BISONS[${#BISONS[@]}-1]}"
        ;;
esac
