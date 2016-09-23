# AccumServer

Очень, очень простой сервер (просто приёмо-передатчик).

## Сборка

Для сборки необходимо наличие компилятора g++ с поддержкой C++11.

Стандартный вариант сборки:
make install

Создаст в каталоге проекта каталог bin с бинарными файлами, необходимыми для запуска отладочной версии сервера (AccumServer_debug). Внимание: перед запуском убедитесь, что в каталоге запуска есть конфигурационный файл accumserver.json.

Сборка релизной версии:
make install build_type=release

Аналогична предыдущему варианту, но запускающий бинарный файл - AccumServer. В этой версии уменьшено количество выводимой информации.

Для изменения каталога сборки укажите переменную bin:
make install bin=/other/directory

## Использование

Чтобы активировать сервер запустите файл AccumServer(_debug). При запуске приложение читает файл accumserver.json в катаоге запуска.

Пример файла accumserver.json с комментариями:

```
{
	"ipAddress":"192.168.0.221",		- ip адрес, подключения к которому будет слушать сервер
	"port":1028,						- порт, который будет прослушиваться сервером
	"maxClientsNum":5,					- максимальное число активных подключений
	"pipePath":"pipes",					- путь к каталогу, в котором будут создаваться каналы общения
	"blacklist":[						- чёрный список - список ip адресов, которым нельзя подключаться. При наличии непустого белого списка опция неактивна
	],
	"whitelist":[						- белый список - список ip которым можно подключаться. Остальным нельзя.
		"192.168.0.17"
	],
	"workProgram":{
		"program":"main.py",			- путь к программе, обрабатывающей клиентские запросы
		"args":[						- аргументы обрабатывающей программы
		]
	}
}
```

## Обрабатывающая программа

Представленный сервер не занимается обработкой принимаемых данных. После приёма блока данных он передаёт их в именованный канал и ожидает получить ответ из другого именованного канала. На другом конце каналов должна находиться обрабатывающая программа, настроенная на эти каналы.

Пример исходного кода обрабатывающей программы на языке Python3:

```python
#!/usr/local/bin/python3

import os, json, stat

with open('accumserver.json') as settings_file:
	settings = json.load(settings_file)

input_pipe_name = settings['pipePath'] + '/inputPipe_' + str(os.getpid())
output_pipe_name = settings['pipePath'] + '/outputPipe_' + str(os.getpid())

while not os.path.exists(input_pipe_name):
	pass
while not os.path.exists(output_pipe_name):
	pass

while True:
	pipe_descr = os.open(input_pipe_name, os.O_RDONLY)
	mes = os.read(pipe_descr, 1000)
	os.close(pipe_descr)
	mes = '<html><body><div>Good-good!</div></body></html>\n'
	mes = 'HTTP/1.1 200 OK\nContent-type:text/html\nContent-length: ' + str(len(mes)) + '\r\n\r\n' + str(mes)
	pipe_descr = os.open(output_pipe_name, os.O_WRONLY)
	os.write(pipe_descr, bytes(mes, 'UTF-8'))
	os.close(pipe_descr)
```
