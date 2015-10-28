using UnityEngine;
using System.Collections;

public class WallCrawlerTarget : MonoBehaviour
{

	public void Start()
	{
		this.transform.parent = null;
	}

	public void OnDrawGizmos()
	{
		Gizmos.color = Color.green;
		Gizmos.DrawSphere(this.transform.position, 0.1f);
	}
}
