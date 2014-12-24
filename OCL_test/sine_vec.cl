__kernel void sine_gpu (__global int* src, __global float* res)
{
  const int hx = get_global_size(0);

  const int idx = get_global_id(0);
  const int idy = get_global_id(1);
  int tmp = idx;

  const float angle = (float) src[idy * hx + idx] / 50.0f;
  while (tmp < hx)
        tmp++;
  res[idy * hx + idx] = 10.0f * sin(angle * 2.0f * M_PI_F);
}

