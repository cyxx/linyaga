
class SceneEvents:
	SCENE_RUN = 1
	SCENE_STOP = 2
	SCENE_ADVANCE = 3

class Point(object):
	def __init__(self, x = 0, y = 0, z = 0):
		self.x = x
		self.y = y
		self.z = z

class Scene(object):
	def __init__(self):
		pass
	def CreatePointCollider(self, x, y, z):
		#print('STUB: Scene.CreatePointCollider')
		return Point(x, y, z)

class ISceneEventSink(object):
	def __init__(self):
		pass
	def Event(self, eventType, data):
		pass

class SceneManagerImpl(object):
	def __init__(self):
		pass
	def CreateScene(self):
		#print('STUB: SceneManagerImpl.CreateScene')
		return Scene()

g_sceneManager = SceneManagerImpl()

def SceneManager():
	return g_sceneManager
