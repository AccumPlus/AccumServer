#!/usr/local/bin/python3

import os, json, stat, signal, threading

def main():
	with open('accumserver.json') as settings_file:
		settings = json.load(settings_file)

	input_pipe_name = settings['pipePath'] + '/inputPipe_' + str(os.getpid())
	output_pipe_name = settings['pipePath'] + '/outputPipe_' + str(os.getpid())

	while not os.path.exists(input_pipe_name) or not os.path.exists(output_pipe_name):
		pass

	while True:
		pipe_descr = os.open(input_pipe_name, os.O_RDONLY)
		mes = os.read(pipe_descr, 1000)
		os.close(pipe_descr)
		reply = '<html><head><title>400 Bad request</title></head><body><h1>400: Bad request</h1></body></html>'
		reply = 'HTTP/1.1 400 Bad Request\nContent-type: text/html\nContent-length: ' + str(len(reply.encode('UTF-8'))) + '\r\n\r\n' + reply
		pipe_descr = os.open(output_pipe_name, os.O_WRONLY)
		os.write(pipe_descr, bytes(reply, 'UTF-8'))
		os.close(pipe_descr)

def sigterm_handler(signal, frame):
	print('Sigterm got!')
	os._exit(0)

def sigint_handler(signal, frame):
	pass

if __name__ == "__main__":
	# Обработчики сигналов
	signal.signal(signal.SIGTERM, sigterm_handler)
	signal.signal(signal.SIGINT, sigint_handler)
	
	thread = threading.Thread(target=main)
	thread.start()
	thread.join()
