using UnityEngine;
using System.Collections;

public class ShortLaser : MonoBehaviour
{

	void Start ()
	{
		this.transform.Rotate(new Vector3(0, 0, Random.Range(0, 360)));
	}

	void Update ()
	{
		this.transform.Rotate(new Vector3(0, 0, 0.1f));
	}
}
