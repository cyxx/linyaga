
import random
import yagaevents
import yagagraphics
import yagahost
import yagascene

class LayerFlags:
	LF_LAYER_ON = 1 << 0

class BinkHandler(object):
	def __init__(self):
		pass

class ISprite(object):
	def __init__(self):
		pass

class Sprite(ISprite):
	def __init__(self):
		ISprite.__init__(self)
		self.position = yagascene.Point()
		self.renderRect = yagagraphics.Rect()
		self.renderMask = 0 | 1
		self.opacity = 1.
		self.currentFrame = 0
		self.frameCount = 1
		self.loopCount = 0
		self._anim = None
		self._callback = None
	def Render(self, camera):
		# assert camera.target == RenderTarget
		# print('Sprite.Render anim:' + str(self.anim) + ' data:' + str(self.__dict__) + ' pos:' + str(self.position))
		if self.anim.res:
			assert isinstance(self.position, yagascene.Point)
			x = int(self.position.x)
			y = int(self.position.y)
			r = yagahost.DrawAnimationFrame(self.anim.res.num, self.currentFrame, self.opacity, self.renderMask, x, y)
			if r:
				self.renderRect = yagagraphics.Rect(r['x'], r['y'], r['w'], r['h'])
	def Seek(self, timeOffset):
		#print('Sprite.Seek')
		frame = self.currentFrame + 1 # TODO: advance by time
		if frame >= self.frameCount:
			frame = 0
			if self.loopCount != 0:
				self.loopCount -= 1
				if self.loopCount == 0:
					if self._callback:
						self._callback.Event(yagascene.SceneEvents.SCENE_STOP, self)
					return
		self.currentFrame = frame
		if self._callback:
			self._callback.Event(yagascene.SceneEvents.SCENE_ADVANCE, self)
	def Intersect(self, pt):
		assert isinstance(pt, yagascene.Point)
		assert pt.z == 0
		r = self.renderRect
		return pt.x >= r.x and pt.x < r.x + r.width and pt.y >= r.y and pt.y < r.y + r.height
	def SetLayerFlag(self, name, flags, flag):
		assert flags == LayerFlags.LF_LAYER_ON
		if self.anim.res:
			yagahost.EnableAnimationFrameLayer(self.anim.res.num, self.currentFrame, name, flag)
	def RegisterEventSink(self, callback):
		#print('Sprite.RegisterEventSink ' + str(callback))
		self._callback = callback
	def UnregisterEventSink(self, callback):
		#print('Sprite.UnregisterEventSink ' + str(callback))
		self._callback = None
	def Stop(self, scene):
		#print('Sprite.Stop scene:' + str(scene))
		if self._callback:
			self._callback.Event(yagascene.SceneEvents.SCENE_STOP, self)
	def Run(self, scene):
		#print('Sprite.Run scene:' + str(scene))
		assert self.anim.framesPerSecond != 0
		if self._callback:
			self._callback.Event(yagascene.SceneEvents.SCENE_RUN, self)
	def getanim(self):
		return self._anim
	def setanim(self, a):
		self._anim = a
		if a.res:
			self.frameCount = yagahost.GetAnimationFramesCount(a.res.num)
		else:
			self.frameCount = 0
		# print('WARNING: Sprite.setanim ' + str(a) + ' framesCount:' + str(self.frameCount))
	anim = property(getanim, setanim)

class TalkieSpriteTalkiesQueue(object):
	def __init__(self):
		self._queue = []
	def __iadd__(self, sound):
		#print('STUB: TalkieSpriteTalkiesQueue.__iadd__ sound:' + str(sound))
		self._queue.append(sound)
		return self
	def __getitem__(self, key):
		if key < len(self._queue):
			return self._queue[key]
		return None
	def __setitem__(self, value, key):
		if key < len(self._queue):
			self._queue[key] = value
	def Clear(self):
		self._queue = []
	def Empty(self):
		return len(self._queue) == 0

class TalkieSpriteTalkies(object):
	def __init__(self):
		self.sounds = TalkieSpriteTalkiesQueue()
		self.duration = 1.0
		self.isPlaying = False
		self._scene = None
	def Clear(self):
		#print('STUB: TalkieSpriteTalkies.Clear')
		self.sounds.Clear()
	def Stop(self, scene):
		#print('STUB: TalkieSpriteTalkies.Stop')
		self._scene = None
		self.isPlaying = False
	def Seek(self, timeOffset):
		if not self.sounds.Empty():
			if not self.sounds._queue[0].isPlaying:
				self.sounds._queue.pop()
				if self.sounds.Empty():
					self.isPlaying = False
				else:
					self.sounds._queue[0].Run(self._scene)
	def Run(self, scene):
		self._scene = scene
		#print('STUB: TalkieSpriteTalkies.Run')
		if not self.sounds.Empty():
			self.sounds._queue[0].Run(scene)
			self.isPlaying = True

RANDOM_PHONEME = True
PHONEMES = { 'A' : 2, 'EE' : 4, 'OH' : 8, 'U' : 16, 'D' : 32, 'M' : 128, 'TH' : 256, 'F' : 512, 'UH' : 1024 }

class TalkieSprite(Sprite):
	def __init__(self):
		Sprite.__init__(self)
		self.talkies = TalkieSpriteTalkies()
		self._talkieStream = None
		self._time = 0
		self._rnd = random.Random()
	def AddChild(self, stream):
		# print('STUB: TalkieSprite.AddChild stream:' + str(stream))
		self._talkieStream = stream
	def RemoveChild(self, stream):
		# print('STUB: TalkieSprite.RemoveChild stream:' + str(stream))
		if self._talkieStream:
			assert self._talkieStream == stream
			self._talkieStream = None
	def Seek(self, timeOffset):
		Sprite.Seek(self, timeOffset)
		self._time += timeOffset
		if self.talkies.isPlaying:
			self.talkies.Seek(timeOffset)
		if self._talkieStream:
			if RANDOM_PHONEME:
				self.renderMask = self._rnd.choice(PHONEMES.values())
				return
			mask = self._talkieStream.MaskData(int(self._time))
			if mask:
				self.renderMask = mask

class IVideoElement(object):
	def __init__(self, res):
		self.isPlaying = False
	def Stop(self, scene):
		print('STUB: IVideoElement.Stop')
	def Run(self, scene):
		print('STUB: IVideoElement.Run')
	def Render(self, camera):
		print('STUB: IVideoElement.Render')
