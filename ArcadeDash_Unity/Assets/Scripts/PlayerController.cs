using UnityEngine;
using System.Collections;

public class PlayerController : MonoBehaviour
{

	public static PlayerController instance;

	public float baseSpeed;
	public int enemiesKilled = 1;


	private Vector2 dirMoving;
	private Renderer attachedRend;
	private bool isDashing;


	void Start ()
	{
		instance = this;
		
		attachedRend = this.GetComponent<Renderer>();
	}

	void Update ()
	{

		if (Input.GetAxis("Horizontal") > 0)
		{
			dirMoving = new Vector2(1, 0);
		}
		if (Input.GetAxis("Horizontal") < 0)
		{
			dirMoving = new Vector2(-1, 0);
		}
		if (Input.GetAxis("Vertical") > 0)
		{
			dirMoving = new Vector2(0, 1);
		}
		if (Input.GetAxis("Vertical") < 0)
		{
			dirMoving = new Vector2(0, -1);
		}

		if (Input.GetButtonDown("Stop"))
		{
			dirMoving = new Vector2(0, 0);
		}

		float currentSpeed = baseSpeed + ((enemiesKilled - 1) * 0.05f);
		if (Input.GetButton("Boost"))
		{
			isDashing = true;
			attachedRend.material.color = Color.red;
			currentSpeed += 0.5f;
		}
		else
		{
			isDashing = false;
			attachedRend.material.color = Color.white;
		}


		Vector3 newPos = this.transform.position + new Vector3(dirMoving.x * currentSpeed * 0.15f, dirMoving.y * currentSpeed * 0.15f, 0);
		this.transform.position = newPos;
	}

	public void PlayerKill()
	{
		Application.LoadLevel(0);
	}

	public void OnCollisionEnter2D(Collision2D coll)
	{
		if (coll.gameObject.GetComponent<Enemy>())
		{
			if (isDashing)
			{
				Destroy(coll.gameObject);
				enemiesKilled++;
			}
			else
			{
				PlayerKill();
			}
		}

		if (coll.gameObject.GetComponent<Wall>() || 
		    coll.gameObject.GetComponent<Laser>())
		{
			PlayerKill();
		}
	}
}
