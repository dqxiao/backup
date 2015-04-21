CREATE FUNCTION overpaid (e emplpyee) RETURNS boolean
  AS $$
    if e["salary"] > 200000:
          return True
    if (e["age"] < 30) and (e["salary"] > 100000):
          return True
    return False
$$ LANGUAGE plpythonu;
