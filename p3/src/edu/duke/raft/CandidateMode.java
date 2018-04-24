package edu.duke.raft;

import java.util.Arrays;
import java.util.Timer;
import java.util.concurrent.ThreadLocalRandom;

public class CandidateMode extends RaftMode {
	Timer electionTimer;
	private final int electionTimerID = 2;

	public void go () {
		synchronized (mLock) {
			int term = mConfig.getCurrentTerm();     
			term++;
			System.out.println ("S" + 
					mID + 
					"." + 
					term + 
					": switched to candidate mode.");
			mConfig.setCurrentTerm(term, mID);
			RaftResponses.clearVotes(term);
			RaftResponses.setTerm(term);

			int electionTimeout; 
			if ((electionTimeout = mConfig.getTimeoutOverride()) == -1) {
				electionTimeout = ThreadLocalRandom.current().nextInt(ELECTION_TIMEOUT_MIN, ELECTION_TIMEOUT_MAX);
			}

			electionTimer = scheduleTimer(electionTimeout, electionTimerID);

			for (int serverID = 1; serverID <= mConfig.getNumServers(); serverID++) {
				this.remoteRequestVote(serverID, mConfig.getCurrentTerm(), mID, mLog.getLastIndex(), mLog.getLastTerm());
			}
		}
	}

	private void electionStart() {
		int term = mConfig.getCurrentTerm();     
		term++;
		System.out.println ("S" + 
				mID + 
				"." + 
				term + 
				": starting new election.");

		mConfig.setCurrentTerm(term, mID);
		RaftResponses.clearVotes(term);
		RaftResponses.setTerm(term);

		for (int serverID = 1; serverID <= mConfig.getNumServers(); serverID++) {
			this.remoteRequestVote(serverID, mConfig.getCurrentTerm(), mID, mLog.getLastIndex(), mLog.getLastTerm());
		}

		int electionTimeout;
		if ((electionTimeout = mConfig.getTimeoutOverride()) == -1) {
			electionTimeout = ThreadLocalRandom.current().nextInt(ELECTION_TIMEOUT_MIN, ELECTION_TIMEOUT_MAX);
		}

		electionTimer = scheduleTimer(electionTimeout, electionTimerID);
	}


	// @param candidate’s term
	// @param candidate requesting vote
	// @param index of candidate’s last log entry
	// @param term of candidate’s last log entry
	// @return 0, if server votes for candidate; otherwise, server's
	// current term 
	public int requestVote (int candidateTerm,
			int candidateID,
			int lastLogIndex,
			int lastLogTerm) {
		synchronized (mLock) {
			log("Vote Request from Server: " + candidateID + "." + candidateTerm);
			
			int currentTerm = mConfig.getCurrentTerm ();

			if (candidateID == mID) {
				log("Voted for Server: " + candidateID + "." + candidateTerm);
				mConfig.setCurrentTerm(candidateTerm, candidateID);
				return 0;
			} else {
				log("Didn't vote for Server: " + candidateID + "." + candidateTerm);
				return currentTerm;
			}
		}
	}


	// @param leader’s term
	// @param current leader
	// @param index of log entry before entries to append
	// @param term of log entry before entries to append
	// @param entries to append (in order of 0 to append.length-1)
	// @param index of highest committed entry
	// @return 0, if server appended entries; otherwise, server's
	// current term
	public int appendEntries (int leaderTerm,
			int leaderID,
			int prevLogIndex,
			int prevLogTerm,
			Entry[] entries,
			int leaderCommit) {
		synchronized (mLock) {
			int term = mConfig.getCurrentTerm ();
			int result = term;
			return result;
		}
	}

	// @param id of the timer that timed out
	public void handleTimeout (int timerID) {
		synchronized (mLock) {
			if (timerID == electionTimerID) {
				//int[] responseVotes = RaftResponses.getVotes(mConfig.getCurrentTerm());
				double voteCount = 0;
				for (int v : RaftResponses.getVotes(mConfig.getCurrentTerm())) {
					if (v == 0) {
						voteCount++;
					} else if (v > mConfig.getCurrentTerm()) {
						RaftServerImpl.setMode(new FollowerMode());
						return;
					}
				}
				log ("RaftResponses.getVotes(): " + Arrays.toString(RaftResponses.getVotes(mConfig.getCurrentTerm())));
				//log ("Vote Count: " + voteCount);
				if (voteCount > ((double)mConfig.getNumServers())/2.0) {
					RaftServerImpl.setMode(new LeaderMode());
				}

				electionStart();
			}
		}
	}
	
	private void log(String message) {
		int currentTerm = mConfig.getCurrentTerm();
		System.out.println("S" + mID + "." + currentTerm + " (Candidate Mode): " + message);
	}
}
