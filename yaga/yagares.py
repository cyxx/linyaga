
import os.path
import yagaevents
import yagahost

g_isfile = os.path.isfile

def YagaAssetExists(f):
	if f.startswith('interface') and yagahost.HasAsset(f):
		return 1
	return g_isfile(f)

os.path.isfile = YagaAssetExists

class ResourceData(object):
	def __init__(self, path, num):
		self.path = path
		self.num = num
	def __del__(self):
		# print('STUB: ResourceData.__del__ path:' + self.path + ' num:' + str(self.num))
		yagahost.FreeAsset(self.num)

class ResourceHandle(object):
	def __init__(self, path):
		self.path = path
	def isLoaded(self):
		print('STUB: ResourceHandle.isLoaded path:' + self.path)
		return False

class ResourceStream(object):
	def __init__(self, path, f):
		self.path = path
		self.f = f

class ResourceManagerImpl(object):
	def __init__(self):
		self.cacheBytes = 0
	def RegisterFormatHandler(self, handler):
		pass
	def UnregisterFormatHandler(self, name):
		pass
	def FlushPreloadQueue(self):
		print('STUB: ResourceManagerImpl.FlushPreloadQueue')
	def Preload(self, path):
		print('STUB: ResourceManagerImpl.Preload path:' + path)
		return ResourceHandle(path)
	def Load(self, path):
		if path.endswith('.evt'):
			# load .evb
			return yagahost.OpenAsset(path[:-1] + 'b')
		if path.endswith('.wav') or path.endswith('.mp3'):
			f = yagahost.OpenAsset(path)
			if f:
				return ResourceStream(path, f)
			return None
		# .mng, .rle
		num = yagahost.LoadAsset(path)
		if num >= 0:
			return ResourceData(path, num)
		return None
	def LoadStream(self, path):
		f = yagahost.OpenAsset(path)
		if f:
			return ResourceStream(path, f)
	def Save(self, name, path):
		print('STUB: ResourceManagerImpl.Save name:' + name + ' path:' + path)
	def CreateEventStreamPlayback(self, name):
		# print('STUB: ResourceManagerImpl.CreateEventStreamPlayback name:' + name)
		return yagaevents.EventStreamPlayback(name)

g_resourceManager = ResourceManagerImpl()

def ResourceManager():
	return g_resourceManager
