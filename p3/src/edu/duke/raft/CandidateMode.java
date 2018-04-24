package edu.duke.raft;

import java.util.Arrays;
import java.util.Timer;
import java.util.concurrent.ThreadLocalRandom;

public class CandidateMode extends RaftMode {
	Timer electionTimer;
	private final int electionTimerID = 2;

	public void go () {
		synchronized (mLock) {
			log("entering candidate mode.");
			electionStart();
		}
	}

	private void electionStart() {
		log("starting new election.");
		mConfig.setCurrentTerm(mConfig.getCurrentTerm() + 1, mID);
		int term = mConfig.getCurrentTerm();
		
		RaftResponses.clearVotes(term);
		RaftResponses.setTerm(term);
		
		int electionTimeout;
		if ((electionTimeout = mConfig.getTimeoutOverride()) == -1) {
			electionTimeout = ThreadLocalRandom.current().nextInt(ELECTION_TIMEOUT_MIN, ELECTION_TIMEOUT_MAX);
		}
		
		electionTimer = scheduleTimer(electionTimeout, electionTimerID);

		for (int serverID = 1; serverID <= mConfig.getNumServers(); serverID++) {
			this.remoteRequestVote(serverID, mConfig.getCurrentTerm(), mID, mLastApplied, mLog.getLastTerm());
		}
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
				log("Voted for self: " + candidateID + "." + candidateTerm);
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
			if (leaderTerm >= mConfig.getCurrentTerm()) {
				electionTimer.cancel();
				mConfig.setCurrentTerm(leaderTerm,leaderID);
				RaftServerImpl.setMode(new FollowerMode());
				return -1;
			}
			return mConfig.getCurrentTerm();
		}
	}

	// @param id of the timer that timed out
	public void handleTimeout (int timerID) {
		synchronized (mLock) {
			if (timerID == electionTimerID) {

				double voteCount = 0;
				for (int v : RaftResponses.getVotes(mConfig.getCurrentTerm())) {
					if (v == 0) {
						voteCount++;
					}
				}
				log ("RaftResponses.getVotes(): " + Arrays.toString(RaftResponses.getVotes(mConfig.getCurrentTerm())));
				
				if (voteCount > (mConfig.getNumServers())/2.0) {
					log("server won, transitioning to leader mode.");
					electionTimer.cancel();
					RaftServerImpl.setMode(new LeaderMode());
				} else {
					electionStart();
				}
			}
		}
	}
	
	private void log(String message) {
		int currentTerm = mConfig.getCurrentTerm();
		System.out.println("S" + mID + "." + currentTerm + " (Candidate Mode): " + message);
	}
}
