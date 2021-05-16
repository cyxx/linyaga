
import yagahost
import yagascene

class ISound(object):
	def __init__(self, res):
		self.res = res
		self.volume = 1.0
		self.position = yagascene.Point(0.5)
		self._sound = -1
	def __del__(self):
		yagahost.StopAudio(self.res.f, self._sound)
	def Run(self, scene):
		# print('STUB: IAudio.Run res:' + str(self.res))
		yagahost.StopAudio(self.res.f, self._sound)
		self._sound = yagahost.PlayAudio(self.res.f, self.res.path)
	def Stop(self, scene):
		# print('STUB: IAudio.Stop')
		yagahost.StopAudio(self.res.f, self._sound)
		self._sound = -1
	def getisplaying(self):
		return yagahost.IsAudioPlaying(self._sound)
	isPlaying = property(getisplaying)

class SoundSystemImpl(object):
	def __init__(self):
		pass

g_soundSystem = SoundSystemImpl()

def SoundSystem():
	return g_soundSystem
