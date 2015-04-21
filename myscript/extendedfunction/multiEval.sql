create or replace function multiEval(t testbig,g text) returns text
AS $$
  def lge(s,t):
    if s<t:
        return True
    else:
        return False


  def eval(t,lhs):
    
    lgeOp=lhs.find('<')

    if lgeOp!=-1:
        attrs=lhs[0:lgeOp]
        attrs=attrs.lstrip()
        value=lhs[lgeOp+1:]
        value=float(value)
        return lge(t[attrs],value)

  
  conditions=g.split(',') 
  result=''
  count=1
  length=len(conditions)
  for condition in conditions:
    if(condition.find('->')==-1):
        continue;
    [lhs,rhs]=condition.split('->')

    if eval(t,lhs):
        result+=rhs
        if(count!=length):
          result+='+'
    
    count+=1

  return result 
  

$$LANGUAGE plpythonu;
