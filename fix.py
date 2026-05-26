path = 'C:/game-in-cpp-full-course/src/gameLayer/gameLayer.cpp'
with open(path, 'r', encoding='utf-8', errors='ignore') as f:
    lines = f.readlines()
new_lines = []
for line in lines:
    if 'raudio.h' in line:
        line = '//' + line
    new_lines.append(line)
with open(path, 'w', encoding='utf-8') as f:
    f.writelines(new_lines)
print('Done!')
