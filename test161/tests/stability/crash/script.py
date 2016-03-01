import os,sys
import subprocess

RANGE=[chr(i) for i in range(ord('a'), ord('p'))]

# First create crash-a/b/c.t files
template = open('crash-template').read()

def add_sys161_opts(content, sys161_opts):
	content = content.split('\n')
	idx = content[1:].index('---') + 1
	content.insert(idx, sys161_opts.strip())
	content = '\n'.join(content)
	return content

def create_crash(char, sys161_opts=None):
	content = template.replace('-template', '-%s' % char)
	content = content.replace(', crash]', ', crash-fork]')
	content = content.replace('[console]', '[console, /asst2/process/forktest.t]')
	if sys161_opts:
		content = add_sys161_opts(content, sys161_opts)

	with open('crash-%s.t' % char, 'w') as f:
		f.write(content)

	# now do the F version
	content = template.replace('-template', '-%sF' % char)
	if sys161_opts:
		content = add_sys161_opts(content, sys161_opts)
	with open('crash-%sF.t' % char, 'w') as f:
		f.write(content)

for char in RANGE:
	create_crash(char)

# Now do the 'all'
sys161_opts = \
'''
sys161:
  ram: 2M
'''
create_crash('all', sys161_opts)

#p = subprocess.Popen('ls *.t', shell=True, stdout=subprocess.PIPE)
#stdout, stderr = p.communicate()
#files = stdout.strip().split('\n')
#
#for f in files:
#	name, ext = os.path.splitext(f)
#
#	new_file = name + 'F' + ext
#	letters = name[name.index('-'):]
#	subprocess.check_call('cp %s %s' % (f, new_file), shell=True)
#	subprocess.check_call('''sed -i 's/%s/%sF/g' %s''' % (letters, letters, new_file), shell=True)
