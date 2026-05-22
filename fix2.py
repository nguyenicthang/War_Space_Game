path = 'C:/game-in-cpp-full-course/src/gameLayer/gameLayer.cpp'
with open(path, 'r', encoding='utf-8', errors='ignore') as f:
    lines = f.readlines()
audio_keywords = ['raudio', 'Sound', 'Music', 'Audio', 'LoadSound', 'PlaySound', 
                  'IsSoundPlaying', 'SetSoundVolume', 'LoadMusicStream', 
                  'PlayMusicStream', 'UpdateMusicStream', 'shootSound', 'music']
new_lines = []
for line in lines:
    stripped = line.strip()
    if any(k in stripped for k in audio_keywords) and not stripped.startswith('//'):
        line = '//' + line
    new_lines.append(line)
with open(path, 'w', encoding='utf-8') as f:
    f.writelines(new_lines)
print('Done!')
