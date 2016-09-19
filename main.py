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
	print('Got' + str(mes))
	pipe_descr = os.open(output_pipe_name, os.O_WRONLY)
	os.write(pipe_descr, bytes('Got!', 'UTF-8'))
	os.close(pipe_descr)
