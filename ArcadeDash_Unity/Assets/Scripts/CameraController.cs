using UnityEngine;
using System.Collections;

public class CameraController : MonoBehaviour
{

	public static CameraController instance;

	public PlayerController player;

	void Start ()
	{
		instance = this;
	}

	void Update ()
	{
		Vector3 targetPos = new Vector3(player.transform.position.x, player.transform.position.y, -1);
		this.transform.position = Vector3.Lerp(this.transform.position, targetPos, 0.5f);
	}
}
