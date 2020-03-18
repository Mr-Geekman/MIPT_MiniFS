# MiniFS

Модель операционной системы, работающая в одном файле.

## Управление

1. Форматирование.
```bash
minifs init
```

2. Создание файла/каталога

```bash
minifs create file "/documents/new_file.txt"
minifs create dir "/documents/new_dir"
```

3. Чтение файла/каталога, результат попадает на стандартный поток вывода
```bash
minifs read file "/documents/new_file.txt"
minifs read dir "/documents/new_dir"
```

4. Удаление файла/каталога, для удаления каталог должен быть пустым
```bash
minifs delete file "/documents/new_file.txt"
minifs delete dir "/documents/new_dir"
```

5. Запись файла, осуществляется со стаднартного потока ввода
```bash
minifs write "/documents/new_file.txt"
```

Соответственно, если вы хотите сохранить файл из MiniFS себе на диск, то нужно запустить команду
```bash
minifs read "/documents/new_file.txt" > my_file.txt
```

## Требования

1. Linux
2. Наличие программы `/usr/bin/realpath`, обладающей флагом -m.
