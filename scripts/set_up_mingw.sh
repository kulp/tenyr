if [[ $PLATFORM = mingw && $TRAVIS_OS_NAME != windows ]]
then
    export USE_WINE=1
    sed "s#{{{}}}#z:$(echo $PWD/3rdparty/sdl2/$HOST-w64-mingw32/bin |
        sed 's#/#\\\\\\\\#g')#" scripts/winepath.reg > scripts/winepath2.reg
    wine regedit scripts/winepath2.reg
fi
