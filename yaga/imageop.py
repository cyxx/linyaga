def scale(data, bpp, srcw, srch, tgtw, tgth):
	size = tgtw * tgth
	return '\x00' * size
