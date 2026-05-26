path = 'C:/game-in-cpp-full-course/src/platform/glfwMain.cpp'
with open(path, 'r', encoding='utf-8', errors='ignore') as f:
    lines = f.readlines()
new_lines = []
for i, line in enumerate(lines):
    if any(k in line for k in ['InitAudioDevice', 'CloseAudioDevice', 'UpdateMusicStream', 'PlayMusicStream', 'LoadMusicStream']):
        line = '//' + line
    new_lines.append(line)
with open(path, 'w', encoding='utf-8') as f:
    f.writelines(new_lines)
print('Done!')
