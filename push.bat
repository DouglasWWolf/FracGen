@echo off
set project=fracgen

git add *.cpp *.h  *.txt
git add res\*.*

git add %project%.rc
git add %project%.sln
git add %project%.vcxproj
git add %project%.vcxproj.filters

git add README.md
git add push.bat

git commit -m "See History.h for changes"
git push origin main

