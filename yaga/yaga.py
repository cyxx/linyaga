
class Profiler(object):
	def __init__(self):
		pass
	def AddProfileKey(self, name):
		print('STUB: Profiler.AddProfileKey name:' + name)
	def StartLogSession(self, name):
		# print('STUB: Profiler.StartLogSession name:' + name)
		pass
	def EndLogSession(self, name):
		# print('STUB: Profiler.EndLogSession name:' + name)
		pass
	def StartProfileFrame(self):
		# print('STUB: Profiler.StartProfileFrame')
		pass
	def EndProfileFrame(self):
		# print('STUB: Profiler.EndProfileFrame')
		pass

MINIMUM_MEMORY_REQUIRED = 48000 * 1024

class SystemImpl(object):
	def __init__(self):
		self.systemMemoryTotal = MINIMUM_MEMORY_REQUIRED
		self.systemMemoryAvail = MINIMUM_MEMORY_REQUIRED
		self.counterU32 = 0
		self.profiler = Profiler()
	def CreateUInt32Seq(self):
		counter = self.counterU32
		#self.counterU32 = (self.counterU32 + 1) & 0xFFFFFFFF
		return counter

g_system = SystemImpl()

def System():
	return g_system
