import random, re, os, json
from pathlib import Path

def scatter_(label, mode, x, y):
  return {"type":"scatter", "mode":mode, "name":label, "x":x, "y":y}

def separate_(xy):
  x = []; y = []
  for xyi in xy:
    x.append(xyi[0])
    y.append(xyi[1])
  return (x,y)

def random_uniform(lo, hi):
    return lo + random.random()*(hi-lo)

def is_number(x):
  return isinstance(x, (int, float, complex)) and not isinstance(x, bool)

def grade_check(actual, expctd, tol):
  success = False
  # print(actual, expctd)
  try:
    if type(actual) == list and type(expctd) == list and len(actual) == len(expctd):
      success = True
      for ii in range(len(actual)):
        success &= grade_check(actual[ii], expctd[ii], tol)
    elif type(actual) == dict and type(expctd) == dict:
      success = True
      for key,val in actual.items():
        success &= key in expctd and grade_check(val, expctd[key], tol)
    elif is_number(actual) and is_number(expctd):
      success = abs(actual-expctd) <= tol if (tol > 0) else actual == expctd
    elif type(actual) == type(expctd):
      success = actual == expctd
    elif type(actual) == bool and is_number(expctd): # matlab does 1 and 0 instead of true/false
      success = (1 if actual else 0) == expctd
    elif type(expctd) == bool and is_number(actual): # matlab does 1 and 0 instead of true/false
      success = (1 if expctd else 0) == actual
  except Exception as exc:
    success = False
  return success

def grade_problem(outputs, output, actual):
  # {"actual":, "expected":, "tolerance":, "points_possible":, "points_earned":}
  success = False
  if output in outputs:
    outputs[output]["actual"] = actual
    if 'points_possible' not in outputs[output]:
      outputs[output]["points_possible"] = 0.0
    tolerance = outputs[output].get("tolerance",0.0)
    if 'expected' in outputs[output]:
      success = grade_check(actual, outputs[output]["expected"], tolerance)
    outputs[output]["points_earned"] = outputs[output]["points_possible"] if success else 0.0

def new_scatter_plot(title, xlabel, ylabel, padding_left, padding_right):
  return {
    "data": [],
    "layout":{
      "autosize": True,
      "title": title,
      "xlabel": {"title":xlabel},
      "ylabel": {"title":ylabel},
      "margin":{"l":padding_left, "r":padding_right}
    },
    "config": {"responsive":True}
  }

def sample(xlo, xhi, n, f):
  niters = max(2,n)
  data = []
  for ii in range(niters):
    x = xlo + ii/(niters-1)*(xhi-xlo)
    data.append([x,f(x)])
  return data

def scatter_dataseries(label, color, line_width, marker_size, xy):
  x,y = separate_(xy)
  mode = "markers" if line_width == 0 else "lines" if marker_size == 0 else "lines+markers"
  result = scatter_(label, mode, x, y)
  if marker_size != 0:
    result["marker"] = {"size":marker_size}
    if color != "": result["marker"]["color"] = color
  if line_width != 0:
    result["line"] = {"width":line_width}
    if color != "": result["line"]["color"] = color
  return result

def hsl(h, s, l):
  return "hsl({},{}%,{}%)".format(h,s,l)

def score(pth, total_points, total_score):
  Path('/score.json').write_text(json.dumps({
    'total_points':total_points, 'total_score':total_score
  }))

def grade(host_rslt, clnt_rslt):
  points_possible = 0
  points_earned = 0
  try:
    host_problems = host_rslt.get('problems',[])
    clnt_problems = clnt_rslt.get('problems',[])
    nprob = len(host_problems)
    for i in range(nprob):
      host_outputs = host_problems[i].get('outputs',{})
      clnt_outputs = clnt_problems[i].get('outputs',{}) if len(clnt_problems)==nprob else {}
      for key,host_output in host_outputs.items():
        points_possible += host_output.get('points_possible',0)
        if key in clnt_outputs:
          clnt_outputs[key]['expected'] = host_output.get('expected', host_output.get('actual', None))
          act = clnt_outputs[key].get('actual', None)
          grade_problem(clnt_outputs, key, act)
          points_earned += clnt_outputs[key].get('points_earned',0)
  except Exception as exc:
    print('Grade exception', exc, flush=True)
  return (points_possible, points_earned)

def check_and_populate_files(cal):
  share_data = cal.get('pblc',{}).get('share_data',{})
  rgx = cal.get('prvt',{}).get('get_from_user_and_dont_expose_from_engine_regex',None)
  if rgx is None:
    print('Must have "get_from_user_and_dont_expose_from_engine_regex" field in "prvt"')
    return None
  found = 0
  for f in share_data.get('files',[]):
    print('  sending {} from user code to user machine'.format(f['name']))
    if re.search(rgx,f['name']) is not None: found += 1
  if found == 0:
    Path('/stdout').write_text('No files from the user match the submission requirement (file names match regular expression "{}")'.format(rgx))
    return None
  codepath = os.path.join('templates', share_data['language'])
  already = [os.path.join(codepath,ff['name']) for ff in share_data.get('files',[])]
  for root, dirs, files in os.walk(codepath):
    for f in files:
      fn = os.path.join(root,f)
      fni = re.sub('^(./|.\\\)','', os.path.join(os.path.relpath(root, codepath), f))
      if re.search(rgx,fni) is None and fn not in already:
        share_data['files'].append({'name':fni,'value':Path(fn).read_bytes(),'bytes':True})
        print('  sending {} from host to user machine'.format(fni))
      else:
        print('  not sending {} from host to user machine'.format(fni))
  return share_data
