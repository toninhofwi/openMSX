# $Id$

from os import environ, remove
from os.path import isfile
from subprocess import PIPE, STDOUT, Popen

def writeFile(path, lines):
	out = open(path, 'w')
	try:
		for line in lines:
			print >> out, line
	finally:
		out.close()

class CompileCommand(object):

	def __init__(self, env, executable, flags):
		self.__env = env
		self.__executable = executable
		self.__flags = flags

	def __str__(self):
		return ' '.join(
			[ self.__executable ] + self.__flags + (
				[ '(%s)' % ' '.join(
					'%s=%s' % item
					for item in sorted(self.__env.iteritems())
					) ] if self.__env else []
				)
			)

	def tryCompile(self, log, sourcePath, lines):
		'''Write the program defined by "lines" to a text file specified
		by "path" and try to compile it.
		Returns True iff compilation succeeded.
		'''
		mergedEnv = dict(environ)
		mergedEnv.update(self.__env)

		assert sourcePath.endswith('.cc')
		objectPath = sourcePath[ : -3] + '.o'
		writeFile(sourcePath, lines)

		try:
			try:
				proc = Popen(
					[ self.__executable ] + self.__flags +
						[ '-c', sourcePath, '-o', objectPath ],
					bufsize = -1,
					env = mergedEnv,
					stdin = None,
					stdout = PIPE,
					stderr = STDOUT,
					)
			except OSError, ex:
				print >> log, 'failed to execute compiler: %s' % ex
				return False
			stdoutdata, stderrdata = proc.communicate()
			if stdoutdata:
				log.write(stdoutdata)
				if not stdoutdata.endswith('\n'): # pylint: disable-msg=E1103
					log.write('\n')
			assert stderrdata is None, stderrdata
			if proc.returncode == 0:
				return True
			else:
				print >> log, 'return code from compile command: %d' % (
					proc.returncode
					)
				return False
			return proc.returncode == 0
		finally:
			remove(sourcePath)
			if isfile(objectPath):
				remove(objectPath)
