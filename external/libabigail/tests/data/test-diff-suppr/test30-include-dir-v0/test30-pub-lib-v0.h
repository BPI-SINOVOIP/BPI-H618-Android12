class S
{
  class priv_type;
  priv_type* priv;

  friend int public_function(S*);
};

int public_function(S*);
